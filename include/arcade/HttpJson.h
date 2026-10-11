#pragma once
#include <ArduinoJson.h>
#include <algorithm>
#include <cstdint>
namespace arcade {
// Buffered, bounded adapter for ArduinoJson's stream reader interface.
template <class Transport> class HttpJsonReader {
  Transport &transport;
  size_t limit, received = 0;
  uint32_t (*clock)();
  uint32_t started;
  char buffer[512];
  int position = 0, available = 0;
  bool failed = false;
public:
  HttpJsonReader(Transport &t, size_t max, uint32_t (*now)())
      : transport(t), limit(max), clock(now), started(now()) {}
  int read() {
    if (position == available) {
      if (uint32_t(clock() - started) > 45000 || received >= limit) {
        failed = true;
        return -1;
      }
      available = transport.read(buffer, int(std::min(sizeof(buffer), limit - received)));
      position = 0;
      if (available <= 0) {
        failed = available < 0;
        return -1;
      }
      received += size_t(available);
    }
    return uint8_t(buffer[position++]);
  }
  size_t readBytes(char *out, size_t count) {
    size_t n = 0;
    for (; n < count; n++) {
      int c = read();
      if (c < 0) break;
      out[n] = char(c);
    }
    return n;
  }
  bool finish(int64_t length) {
    int c;
    while ((c = read()) >= 0)
      if (c != ' ' && c != '\r' && c != '\n' && c != '\t') return false;
    return !failed && transport.complete() &&
           (length <= 0 || uint64_t(length) == received);
  }
};
template <class Document, class Reader>
bool readReleaseJson(Document &out, Reader &reader) {
  StaticJsonDocument<512> filter;
  filter["draft"] = true;
  filter["prerelease"] = true;
  filter["tag_name"] = true;
  filter["assets"][0]["name"] = true;
  filter["assets"][0]["browser_download_url"] = true;
  filter["assets"][0]["size"] = true;
  return !deserializeJson(out, reader, DeserializationOption::Filter(filter));
}
} // namespace arcade
