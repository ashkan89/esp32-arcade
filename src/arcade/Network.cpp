#include "arcade/Network.h"
#include "arcade/ReleasePolicy.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <cstring>
#include <ctime>
#include <esp_app_format.h>
#include <esp_crt_bundle.h>
#include <esp_heap_caps.h>
#include <esp_http_client.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>
// Arduino shadows the SDK header with its custom-bundle-only variant.
// Use the SDK's prebuilt full Mozilla bundle, linked from libmbedtls.
extern "C" esp_err_t esp_crt_bundle_attach(void *conf);
#if !CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
#error This console requires a rollback-enabled SDK and matching bootloader
#endif
namespace arcade {
namespace {
constexpr const char *Board = "esp32-wrover-n16r8-ili9225";
constexpr const char *Binary = "arcade-esp32-n16r8.bin";
struct PsramAllocator {
  void *allocate(size_t n) {
    return heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }
  void deallocate(void *p) {
    heap_caps_free(p);
  }
  void *reallocate(void *p, size_t n) {
    return heap_caps_realloc(p, n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }
};
using Json = BasicJsonDocument<PsramAllocator>;
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
      esp_http_client_config_t config{};
      config.url = url.c_str();
      config.timeout_ms = 15000;
      config.crt_bundle_attach = esp_crt_bundle_attach;
      config.disable_auto_redirect = true;
      config.event_handler = event;
      config.user_data = this;
      config.buffer_size = 4096;
      config.user_agent = "GameArcade/" ARCADE_VERSION;
      client = esp_http_client_init(&config);
      if (!client)
        return false;
      esp_http_client_set_header(client, "Accept", "application/vnd.github+json");
      esp_http_client_set_header(client, "X-GitHub-Api-Version", "2022-11-28");
      if (esp_http_client_open(client, 0) != ESP_OK)
        return false;
      length = esp_http_client_fetch_headers(client);
      if (length < 0)
        return false;
      int code = esp_http_client_get_status_code(client);
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
  bool json(Json &out, size_t limit) {
    if (length > int64_t(limit))
      return false;
    auto *buffer =
        static_cast<char *>(heap_caps_malloc(limit + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!buffer)
      return false;
    size_t used = 0;
    bool ok = true;
    uint32_t start = millis();
    while (used < limit) {
      int n = read(buffer + used, int(std::min(size_t(4096), limit - used)));
      if (n < 0) {
        ok = false;
        break;
      }
      if (!n)
        break;
      used += size_t(n);
      if (elapsed(millis(), start) > 45000) {
        ok = false;
        break;
      }
    }
    if (used == limit || (!esp_http_client_is_complete_data_received(client)) ||
        (length > 0 && uint64_t(length) != used))
      ok = false;
    if (ok)
      ok = !deserializeJson(out, buffer, used);
    heap_caps_free(buffer);
    return ok;
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
      ssid = config["ssid"] | "";
      password = config["password"] | "";
      repository = config["repository"] | "";
    }
  }
  if (repository.isEmpty())
    repository = "ashkan89/esp32-arcade";
  WiFi.persistent(false);
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
  uint32_t a = esp_random(), b = esp_random();
  std::snprintf(apPassword, sizeof(apPassword), "%08lx%04lx", (unsigned long)a,
                (unsigned long)(b & 65535));
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP("GameArcade-Setup", apPassword, 1, false, 1)) {
    status("COULD NOT START SETUP WI-FI");
    WiFi.mode(WIFI_OFF);
    return;
  }
  server.on("/", HTTP_GET, [this] {
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
        "'></label><button>Save and connect</button></form><p>Setup closes after five minutes.</p>";
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "text/html", page);
  });
  server.on("/save", HTTP_POST, [this] {
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
    if (prefs.putString("config", encoded) != encoded.length()) {
      server.send(500, "text/plain", "Could not save settings.");
      return;
    }
    ssid = name;
    password = pass;
    repository = repo;
    found = false;
    savePending = true;
    server.send(200, "text/plain", "Saved. Return to the console and choose Check GitHub release.");
  });
  server.onNotFound([this] { server.send(404, "text/plain", "Open http://192.168.4.1/"); });
  server.begin();
  portal = true;
  portalAt = millis();
  status("SETUP: OPEN 192.168.4.1");
}
void Network::service(bool menu) {
  if (working)
    return;
  if (portal) {
    server.handleClient();
    if (savePending || !menu || elapsed(millis(), portalAt) > 300000) {
      server.stop();
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_OFF);
      portal = false;
      savePending = false;
      status("WI-FI SETTINGS SAVED / READY");
    }
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
    Json release(65536);
    if (!request.open("https://api.github.com/repos/" + repository + "/releases/latest") ||
        !request.json(release, 131072)) {
      status("GITHUB CHECK FAILED / NO RELEASE");
      return false;
    }
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
      if (std::strcmp(name, "arcade-release.json") == 0)
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
  Json manifest(2048);
  if (!request.open(manifestUrl) || !request.json(manifest, 4096)) {
    status("RELEASE MANIFEST DOWNLOAD FAILED");
    return false;
  }
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
  String identity = "SKY_ARCADE_N16R8|" + releaseVersion;
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
      if (header->magic != ESP_IMAGE_HEADER_MAGIC || header->chip_id != ESP_CHIP_ID_ESP32) {
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
  if (connect()) {
    if (job == ConsoleAction::CheckUpdate)
      check();
    else
      install();
  }
  WiFi.disconnect(false);
  WiFi.mode(WIFI_OFF);
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
  if (xTaskCreatePinnedToCore(task, "github-ota", 10240, this, 1, nullptr, 0) != pdPASS) {
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
