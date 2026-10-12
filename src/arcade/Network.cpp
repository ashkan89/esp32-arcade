#include "arcade/Network.h"
#include "arcade/HttpJson.h"
#include "arcade/ReleasePolicy.h"
#include "arcade/SetupKey.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <cstring>
#include <ctime>
#include <esp_app_format.h>
#include <esp_crt_bundle.h>
#include <esp_heap_caps.h>
#include <esp_http_client.h>
#include <esp_ota_ops.h>
#include <esp_tls.h>
#include <mbedtls/sha256.h>
// Arduino shadows the SDK header with its custom-bundle-only variant.
// Use the SDK's prebuilt full Mozilla bundle, linked from libmbedtls.
extern "C" esp_err_t esp_crt_bundle_attach(void *conf);
#if ARCADE_PROFILE
extern "C" int __real_mbedtls_ssl_handshake_client_step(mbedtls_ssl_context *ssl);
extern "C" int __wrap_mbedtls_ssl_handshake_client_step(mbedtls_ssl_context *ssl) {
  int state = ssl->state;
  uint32_t start = millis();
  Serial.printf("[https] Handshake stage=%d\n", state);
  int result = __real_mbedtls_ssl_handshake_client_step(ssl);
  Serial.printf("[https] Stage %d -> %d result=%d duration=%lu ms stack=%u\n", state,
                ssl->state, result, (unsigned long)(millis() - start),
                unsigned(uxTaskGetStackHighWaterMark(nullptr)));
  return result;
}
#endif
#if !CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
#error This console requires a rollback-enabled SDK and matching bootloader
#endif
namespace arcade {
namespace {
constexpr const char *Board = ARCADE_BOARD_ID;
constexpr const char *Binary = ARCADE_BINARY;
esp_err_t attachHttpsTrust(void *conf) {
  esp_err_t result = esp_crt_bundle_attach(conf);
  if (result != ESP_OK)
    return result;
  auto *tls = static_cast<mbedtls_ssl_config *>(conf);
  // GitHub's ECDSA certificate chain takes >8 seconds to verify on classic ESP32.
  // Select its RSA chain and hardware-accelerated RSA verification instead, while
  // retaining ECDHE forward secrecy, AES-GCM, and full CA/hostname verification.
  static const int suites[] = {MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
                               MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384, 0};
  static const mbedtls_ecp_group_id curves[] = {MBEDTLS_ECP_DP_SECP256R1, MBEDTLS_ECP_DP_NONE};
  mbedtls_ssl_conf_min_version(tls, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
  mbedtls_ssl_conf_max_version(tls, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
  mbedtls_ssl_conf_ciphersuites(tls, suites);
  mbedtls_ssl_conf_curves(tls, curves);
  return ESP_OK;
}
struct JsonAllocator {
  void *allocate(size_t n) {
    return heap_caps_malloc(n, (ARCADE_WROOM ? MALLOC_CAP_INTERNAL : MALLOC_CAP_SPIRAM) | MALLOC_CAP_8BIT);
  }
  void deallocate(void *p) {
    heap_caps_free(p);
  }
  void *reallocate(void *p, size_t n) {
    return heap_caps_realloc(p, n, (ARCADE_WROOM ? MALLOC_CAP_INTERNAL : MALLOC_CAP_SPIRAM) | MALLOC_CAP_8BIT);
  }
};
using Json = BasicJsonDocument<JsonAllocator>;
bool trustedUrl(const String &url) {
  if (!url.startsWith("https://"))
    return false;
  int end = url.indexOf('/', 8);
  String host = end < 0 ? url.substring(8) : url.substring(8, end);
  return host == "api.github.com" || host == "github.com" ||
         host == "release-assets.githubusercontent.com" ||
         host == "objects.githubusercontent.com" || host == "github-releases.githubusercontent.com";
}
class Request {
  esp_http_client_handle_t client = nullptr;
  String location;
  static esp_err_t event(esp_http_client_event_t *evt) {
    if (evt->event_id == HTTP_EVENT_DISCONNECTED && evt->data) {
      int tlsCode = 0, flags = 0;
      esp_err_t error = esp_tls_get_and_clear_last_error(
          static_cast<esp_tls_error_handle_t>(evt->data), &tlsCode, &flags);
      if (error || tlsCode || flags)
        Serial.printf("[https] TLS error=%s code=-0x%04x verify=0x%x\n", esp_err_to_name(error),
                      unsigned(tlsCode < 0 ? -tlsCode : tlsCode), flags);
    }
    if (evt->event_id == HTTP_EVENT_ON_HEADER && evt->header_key &&
        strcasecmp(evt->header_key, "Location") == 0)
      static_cast<Request *>(evt->user_data)->location = evt->header_value;
    return ESP_OK;
  }

public:
  int64_t length = -1;
  ~Request() {
    close();
  }
  void close() {
    if (client) {
      esp_http_client_close(client);
      esp_http_client_cleanup(client);
      client = nullptr;
    }
  }
  bool open(String url) {
    for (int redirects = 0; redirects < 5; redirects++) {
      close();
      location = "";
      if (!trustedUrl(url))
        return false;
      int hostEnd = url.indexOf('/', 8);
      String host = hostEnd < 0 ? url.substring(8) : url.substring(8, hostEnd);
      IPAddress address;
      if (!WiFi.hostByName(host.c_str(), address)) {
        Serial.printf("[https] DNS failed: %s\n", host.c_str());
        return false;
      }
      Serial.printf("[https] Connecting %s (%s); internal heap=%u largest=%u\n", host.c_str(),
                    address.toString().c_str(), heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                    heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
      esp_http_client_config_t config{};
      config.url = url.c_str();
      config.timeout_ms = 30000;
      config.crt_bundle_attach = attachHttpsTrust;
      config.disable_auto_redirect = true;
      config.event_handler = event;
      config.user_data = this;
      config.buffer_size = 1024;
      // The SDK's default 512-byte TX buffer must fit the entire request line.
      // GitHub release redirects include signed query strings longer than that.
      config.buffer_size_tx = 4096;
      config.user_agent = "GameArcade/" ARCADE_VERSION;
      client = esp_http_client_init(&config);
      if (!client)
        return false;
      esp_http_client_set_header(client, "Accept", "application/vnd.github+json");
      esp_http_client_set_header(client, "X-GitHub-Api-Version", "2022-11-28");
      uint32_t started = millis();
      esp_err_t opened = esp_http_client_open(client, 0);
      if (opened != ESP_OK) {
        Serial.printf("[https] Open failed: %s errno=%d after %lu ms\n", esp_err_to_name(opened),
                      esp_http_client_get_errno(client), (unsigned long)elapsed(millis(), started));
        return false;
      }
      length = esp_http_client_fetch_headers(client);
      if (length < 0)
        return false;
      int code = esp_http_client_get_status_code(client);
      Serial.printf("[https] HTTP %d after %lu ms\n", code,
                    (unsigned long)elapsed(millis(), started));
      if (code == 200)
        return true;
      if ((code == 301 || code == 302 || code == 303 || code == 307 || code == 308) &&
          location.length()) {
        url = location;
        continue;
      }
      return false;
    }
    return false;
  }
  int read(void *p, int n) {
    return esp_http_client_read(client, static_cast<char *>(p), n);
  }
  bool complete() {
    return esp_http_client_is_complete_data_received(client);
  }
  bool json(Json &out, size_t limit, bool release = false) {
    if (length > int64_t(limit)) return false;
    // Stream parsing copies strings into the document instead of retaining
    // pointers into a mutable response buffer that is about to be freed.
    HttpJsonReader<Request> reader(*this, limit, []() -> uint32_t { return millis(); });
    bool ok = release ? readReleaseJson(out, reader) : !deserializeJson(out, reader);
    return ok && reader.finish(length);
  }
};
String escaped(String value) {
  value.replace("&", "&amp;");
  value.replace("<", "&lt;");
  value.replace("\"", "&quot;");
  value.replace("'", "&#39;");
  return value;
}
} // namespace
void Network::status(const char *text) {
  Serial.printf("[network] %s\n", text);
  portENTER_CRITICAL(&statusLock);
  std::snprintf(message, sizeof(message), "%s", text);
  portEXIT_CRITICAL(&statusLock);
}
void Network::begin() {
  storageReady = prefs.begin("arcade-net", false);
  if (storageReady) {
    String saved = prefs.isKey("config") ? prefs.getString("config", "") : String();
    StaticJsonDocument<512> config;
    if (saved.length() < 512 && !deserializeJson(config, saved)) {
      persistedConfig = saved;
      ssid = config["ssid"] | "";
      password = config["password"] | "";
      repository = config["repository"] | "";
    }
  }
  if (repository.isEmpty())
    repository = "ashkan89/esp32-arcade";
  WiFi.persistent(false);
  WiFi.onEvent([](WiFiEvent_t event) {
    switch (event) {
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      Serial.println("[network] AP client joined");
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      Serial.println("[network] AP client disconnected");
      break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      Serial.println("[network] AP client received DHCP address");
      break;
    default:
      break;
    }
  });
  WiFi.mode(WIFI_OFF);
}
void Network::setupPortal() {
  if (!storageReady) {
    status("NETWORK SETTINGS STORAGE FAILED");
    return;
  }
  if (portal) {
    status("SETUP: OPEN 192.168.4.1");
    return;
  }
  formatSetupKey(apPassword, esp_random(), esp_random());
  const IPAddress ip(192, 168, 4, 1), mask(255, 255, 255, 0);
  if (!WiFi.mode(WIFI_AP) || !WiFi.softAPConfig(ip, ip, mask) ||
      !WiFi.softAP("GameArcade-Setup", apPassword, 1, false, 4)) {
    status("COULD NOT START SETUP WI-FI");
    WiFi.mode(WIFI_OFF);
    return;
  }
  if (!dns.start(53, "*", ip)) {
    status("COULD NOT START SETUP DNS");
    WiFi.mode(WIFI_OFF);
    return;
  }
  // Register once: repeated setup sessions otherwise accumulate WebServer handlers.
  if (!routesRegistered) {
    server.on("/", HTTP_GET, [this] {
      Serial.println("[network] Setup page requested");
      String page =
          "<!doctype html><meta name='viewport' content='width=device-width'><title>Game Arcade "
          "setup</title><style>body{font:18px system-ui;max-width:32em;margin:2em "
          "auto;padding:1em;background:#101729;color:white}input,button{display:block;padding:.7em;"
          "margin:.5em 0;width:90%}</style><h1>Game Arcade</h1><p>Configure Wi-Fi and a public "
          "GitHub "
          "release repository.</p><form method='post' action='/save'>";
      page +=
          "<input name='token' type='hidden' value='" + String(apPassword) +
          "'><label>Wi-Fi name<input name='ssid' maxlength='32' required value='" + escaped(ssid) +
          "'></label><label>Wi-Fi password (enter again)<input name='password' type='password' "
          "minlength='8' maxlength='63' required autocomplete='new-password'></label><label>GitHub "
          "owner/repository<input name='repository' maxlength='100' required "
          "placeholder='owner/repository' value='" +
          escaped(repository) +
          "'></label><button>Save settings</button></form><p>Setup closes after five minutes.</p>";
      server.sendHeader("Cache-Control", "no-store");
      server.send(200, "text/html", page);
    });
    server.on("/save", HTTP_POST, [this] {
      if (savePending) {
        server.send(409, "text/plain", "Settings already saved. Setup is closing.");
        return;
      }
      String name = server.arg("ssid"), pass = server.arg("password"),
             repo = server.arg("repository");
      repo.trim();
      if (server.arg("token") != apPassword || name.length() < 1 || name.length() > 32 ||
          pass.length() < 8 || pass.length() > 63 || !validRepository(repo.c_str())) {
        server.send(
            400, "text/plain",
            "Invalid setup fields. Use owner/repository and an 8-63 character Wi-Fi password.");
        return;
      }
      StaticJsonDocument<512> config;
      config["ssid"] = name;
      config["password"] = pass;
      config["repository"] = repo;
      String encoded;
      serializeJson(config, encoded);
      if (encoded != persistedConfig) {
        uint32_t now = millis();
        if (!configCooldown.eligible(now)) {
          server.sendHeader("Retry-After", "60");
          server.send(429, "text/plain", "Please wait one minute before saving changed settings again.");
          return;
        }
        configCooldown.noteAttempt(now);
        if (prefs.putString("config", encoded) != encoded.length()) {
          // A failed commit makes the persisted value uncertain until a successful retry.
          persistedConfig = "";
          server.send(500, "text/plain", "Could not save settings. Retry in one minute.");
          return;
        }
        persistedConfig = encoded;
      }
      ssid = name;
      password = pass;
      repository = repo;
      found = false;
      savePending = true;
      savedAt = millis();
      Serial.println("[network] Setup settings saved");
      server.send(200, "text/plain",
                  "Saved. Return to the console and choose Check GitHub release.");
    });
    server.onNotFound([this] {
      server.sendHeader("Location", "http://192.168.4.1/", true);
      server.sendHeader("Cache-Control", "no-store");
      server.send(302, "text/plain", "Open http://192.168.4.1/");
    });
    routesRegistered = true;
  }
  server.begin();
  portal = true;
  portalAt = millis();
  status("SETUP: OPEN 192.168.4.1");
}
void Network::closePortal(const char *reason) {
  server.stop();
  dns.stop();
  // softAPdisconnect rewrites the running AP config before stopping it. That can
  // race SDK netstack callbacks with shutdown; mode(OFF) stops clients and radio directly.
  WiFi.mode(WIFI_OFF);
  portal = false;
  savePending = false;
  status(reason);
}
void Network::service(bool menu) {
  if (working)
    return;
  if (portal) {
    dns.processNextRequest();
    server.handleClient();
    // Keep the AP up briefly so the browser receives the save response before teardown.
    if (savePending) {
      if (elapsed(millis(), savedAt) >= 1000)
        closePortal("WI-FI SETTINGS SAVED / READY");
    } else if (!menu)
      closePortal("WI-FI SETUP CANCELLED");
    else if (elapsed(millis(), portalAt) >= 300000)
      closePortal("WI-FI SETUP TIMED OUT");
  } else if (!menu && WiFi.getMode() != WIFI_OFF) {
    WiFi.disconnect(false);
    WiFi.mode(WIFI_OFF);
  }
}
bool Network::connect() {
  if (ssid.isEmpty() || !validRepository(repository.c_str())) {
    status("RUN WI-FI / REPOSITORY SETUP");
    return false;
  }
  status("CONNECTING TO WI-FI");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (elapsed(millis(), start) > 20000) {
      status("WI-FI CONNECTION FAILED");
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
  status("SYNCING CLOCK FOR HTTPS");
  configTime(0, 0, "pool.ntp.org", "time.cloudflare.com");
  start = millis();
  while (std::time(nullptr) < 1704067200) {
    if (elapsed(millis(), start) > 15000) {
      status("CLOCK SYNC FAILED / RETRY");
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  Serial.printf("[https] Clock epoch=%lld\n", static_cast<long long>(std::time(nullptr)));
  return true;
}
bool Network::check() {
  found = false;
  status("CHECKING GITHUB RELEASE");
  String prefix = "https://github.com/" + repository + "/releases/download/";
  String manifestUrl, binaryUrl, tag;
  uint32_t binarySize = 0;
  {
    Request request;
    if (!request.open("https://api.github.com/repos/" + repository + "/releases/latest")) {
      status("GITHUB CHECK FAILED / NO RELEASE");
      return false;
    }
    // Allocate metadata after the TLS handshake's temporary allocations have
    // been released, preserving internal heap margin on no-PSRAM targets.
    Json release(16384);
    if (!request.json(release, 131072, true)) {
      status("GITHUB CHECK FAILED / NO RELEASE");
      return false;
    }
    request.close();
    if ((release["draft"] | false) || (release["prerelease"] | false)) {
      status("ONLY STABLE RELEASES ACCEPTED");
      return false;
    }
    tag = release["tag_name"] | "";
    if (!newerVersion(tag.c_str(), ARCADE_VERSION)) {
      status("CURRENT FIRMWARE IS UP TO DATE");
      return true;
    }
    for (JsonObject asset : release["assets"].as<JsonArray>()) {
      const char *name = asset["name"] | "";
      String url = asset["browser_download_url"] | "";
      if (!url.startsWith(prefix))
        continue;
      if (std::strcmp(name, ARCADE_MANIFEST) == 0)
        manifestUrl = url;
      if (std::strcmp(name, Binary) == 0) {
        binaryUrl = url;
        binarySize = asset["size"] | 0u;
      }
    }
  }
  if (manifestUrl.isEmpty() || binaryUrl.isEmpty()) {
    status("RELEASE ASSETS ARE MISSING");
    return false;
  }
  Request request;
  if (!request.open(manifestUrl)) {
    status("RELEASE MANIFEST DOWNLOAD FAILED");
    return false;
  }
  Json manifest(2048);
  if (!request.json(manifest, 4096)) {
    status("RELEASE MANIFEST DOWNLOAD FAILED");
    return false;
  }
  request.close();
  const char *version = manifest["version"] | "";
  const char *board = manifest["board"] | "";
  const char *file = manifest["asset"] | "";
  uint32_t size = manifest["size"] | 0u;
  Version a, b;
  const esp_partition_t *slot = esp_ota_get_next_update_partition(nullptr);
  if ((manifest["schema"] | 0) != 1 || std::strcmp(board, Board) || std::strcmp(file, Binary) ||
      !parseVersion(version, a) || !parseVersion(tag.c_str(), b) || a.major != b.major ||
      a.minor != b.minor || a.patch != b.patch || !newerVersion(version, ARCADE_VERSION) || !slot ||
      size < 512 || size > slot->size || size != binarySize ||
      !parseSha256(manifest["sha256"] | "", expectedHash)) {
    status("RELEASE BOARD / HASH / SIZE INVALID");
    return false;
  }
  firmwareUrl = binaryUrl;
  releaseVersion = version;
  expectedSize = size;
  found = true;
  status("NEW RELEASE READY / CHOOSE INSTALL");
  return true;
}
bool Network::install() {
  if (!found) {
    status("CHECK FOR A NEW RELEASE FIRST");
    return false;
  }
  const esp_partition_t *slot = esp_ota_get_next_update_partition(nullptr);
  if (!slot || slot == esp_ota_get_running_partition() || expectedSize > slot->size) {
    status("NO INACTIVE OTA SLOT AVAILABLE");
    return false;
  }
  esp_ota_img_states_t state;
  if (esp_ota_get_state_partition(esp_ota_get_running_partition(), &state) == ESP_OK &&
      state == ESP_OTA_IMG_PENDING_VERIFY) {
    status("CURRENT BOOT IS NOT CONFIRMED");
    return false;
  }
  Request request;
  if (!request.open(firmwareUrl) ||
      (request.length > 0 && uint64_t(request.length) != expectedSize)) {
    status("FIRMWARE DOWNLOAD FAILED");
    return false;
  }
  status("DOWNLOADING TO INACTIVE SLOT");
  esp_ota_handle_t handle = 0;
  mbedtls_sha256_context hash;
  mbedtls_sha256_init(&hash);
  mbedtls_sha256_starts_ret(&hash, 0);
  auto *buffer =
      static_cast<uint8_t *>(heap_caps_malloc(4096, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  bool ok = buffer != nullptr, identityFound = false;
  uint32_t received = 0, start = millis();
  int matched = 0;
  String identity = ARCADE_IDENTITY + releaseVersion;
  if (ok)
    ok = esp_ota_begin(slot, expectedSize, &handle) == ESP_OK;
  while (ok && received < expectedSize) {
    int n = request.read(buffer, int(std::min(4096u, expectedSize - received)));
    if (n <= 0) {
      ok = false;
      break;
    }
    if (received == 0) {
      while (n > 0 && n < int(sizeof(esp_image_header_t))) {
        int extra = request.read(buffer + n, int(sizeof(esp_image_header_t)) - n);
        if (extra <= 0) {
          ok = false;
          break;
        }
        n += extra;
      }
      if (!ok || n < int(sizeof(esp_image_header_t))) {
        ok = false;
        break;
      }
      auto *header = reinterpret_cast<esp_image_header_t *>(buffer);
      if (header->magic != ESP_IMAGE_HEADER_MAGIC || header->chip_id != ESP_CHIP_ID_ESP32 ||
          header->spi_size != (ARCADE_FLASH_MB == 4 ? ESP_IMAGE_FLASH_SIZE_4MB : ESP_IMAGE_FLASH_SIZE_16MB)) {
        ok = false;
        break;
      }
    }
    for (int i = 0; i < n; i++) {
      if (matched == int(identity.length())) {
        if (buffer[i] == 0)
          identityFound = true;
        matched = 0;
      }
      if (buffer[i] == uint8_t(identity[matched]))
        matched++;
      else
        matched = buffer[i] == uint8_t(identity[0]) ? 1 : 0;
    }
    if (mbedtls_sha256_update_ret(&hash, buffer, n) != 0 ||
        esp_ota_write(handle, buffer, n) != ESP_OK) {
      ok = false;
      break;
    }
    received += uint32_t(n);
    progress = received * 100u / expectedSize;
    if (elapsed(millis(), start) > 300000) {
      ok = false;
      break;
    }
    vTaskDelay(1);
  }
  uint8_t digest[32]{};
  if (mbedtls_sha256_finish_ret(&hash, digest) != 0)
    ok = false;
  mbedtls_sha256_free(&hash);
  heap_caps_free(buffer);
  ok =
      ok && identityFound && received == expectedSize && std::memcmp(digest, expectedHash, 32) == 0;
  if (!ok) {
    if (handle)
      esp_ota_abort(handle);
    status("UPDATE REJECTED / CURRENT SLOT SAFE");
    return false;
  }
  status("VALIDATING FIRMWARE IMAGE");
  if (esp_ota_end(handle) != ESP_OK || esp_ota_set_boot_partition(slot) != ESP_OK) {
    status("IMAGE VALIDATION / BOOT SELECT FAILED");
    return false;
  }
  status("UPDATE VERIFIED / RESTARTING");
  vTaskDelay(pdMS_TO_TICKS(1000));
  esp_restart();
  return true;
}
void Network::rollback() {
  const auto *running = esp_ota_get_running_partition();
  esp_partition_subtype_t subtype = running->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_0
                                        ? ESP_PARTITION_SUBTYPE_APP_OTA_1
                                        : ESP_PARTITION_SUBTYPE_APP_OTA_0;
  const auto *previous = esp_partition_find_first(ESP_PARTITION_TYPE_APP, subtype, nullptr);
  esp_ota_img_states_t state;
  esp_app_desc_t info;
  if (!previous || esp_ota_get_state_partition(previous, &state) != ESP_OK ||
      state != ESP_OTA_IMG_VALID || esp_ota_get_partition_description(previous, &info) != ESP_OK) {
    status("NO HEALTHY PREVIOUS FIRMWARE");
    return;
  }
  if (esp_ota_set_boot_partition(previous) != ESP_OK) {
    status("PREVIOUS FIRMWARE SELECT FAILED");
    return;
  }
  status("BOOTING PREVIOUS FIRMWARE");
  esp_restart();
}
void Network::work() {
  Serial.printf("[network] Start: free=%u largest=%u\n",
                heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  if (connect()) {
    if (job == ConsoleAction::CheckUpdate)
      check();
    else
      install();
  }
  WiFi.disconnect(false);
  WiFi.mode(WIFI_OFF);
  Serial.printf("[network] Done: free=%u minimum=%u largest=%u stack-unused=%u\n",
                heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                unsigned(uxTaskGetStackHighWaterMark(nullptr)));
  working = false;
}
void Network::task(void *self) {
  static_cast<Network *>(self)->work();
  vTaskDelete(nullptr);
}
void Network::request(ConsoleAction action) {
  if (working || action == ConsoleAction::None)
    return;
  if (action == ConsoleAction::SetupNetwork) {
    setupPortal();
    return;
  }
  if (action == ConsoleAction::Rollback) {
    rollback();
    return;
  }
  if (portal) {
    status("FINISH OR CLOSE WI-FI SETUP FIRST");
    return;
  }
  job = action;
  progress = 0;
  working = true;
  // HTTPS certificate verification can be CPU-bound for several seconds. Share
  // priority with IDLE0 so it can service its watchdog even inside synchronous SDK calls.
  if (xTaskCreatePinnedToCore(task, "github-ota", 10240, this, tskIDLE_PRIORITY, nullptr, 0) != pdPASS) {
    working = false;
    status("NOT ENOUGH MEMORY FOR UPDATE TASK");
  }
}
void Network::draw(Canvas &c) {
  char text[96];
  portENTER_CRITICAL(&statusLock);
  std::memcpy(text, message, sizeof(text));
  portEXIT_CRITICAL(&statusLock);
  for (int row = 0; row < 3; row++) {
    size_t offset = size_t(row * 27);
    if (offset >= std::strlen(text))
      break;
    char line[28]{};
    std::strncpy(line, text + offset, 27);
    c.text(6, 140 + row * 12, line, working ? Cyan : White);
  }
  if (portal) {
    c.text(6, 176, "WI-FI: GameArcade-Setup", Yellow);
    c.text(6, 188, "KEY:", Grey);
    c.text(36, 188, apPassword, Yellow);
    c.text(6, 202, "HTTP://192.168.4.1", White);
  } else {
    c.text(6, 187, "VERSION " ARCADE_VERSION, Grey);
    if (found && !working)
      c.text(6, 203, releaseVersion.c_str(), Green);
    if (working) {
      c.rect(6, 188, 164, 7, Navy);
      c.rect(6, 188, int(progress) * 164 / 100, 7, Green);
    }
  }
}
bool Network::pendingBoot() {
  esp_ota_img_states_t state;
  return esp_ota_get_state_partition(esp_ota_get_running_partition(), &state) == ESP_OK &&
         state == ESP_OTA_IMG_PENDING_VERIFY;
}
bool Network::confirmBoot() {
  const auto *running = esp_ota_get_running_partition();
  esp_ota_img_states_t state;
  esp_err_t result = esp_ota_get_state_partition(running, &state);
  if (result == ESP_OK && state == ESP_OTA_IMG_VALID)
    return true;
  if (result != ESP_OK && esp_ota_set_boot_partition(running) != ESP_OK)
    return false;
  return esp_ota_mark_app_valid_cancel_rollback() == ESP_OK;
}
void Network::failBoot() {
  if (pendingBoot()) {
    esp_ota_mark_app_invalid_rollback_and_reboot();
    esp_restart();
  }
}
} // namespace arcade
