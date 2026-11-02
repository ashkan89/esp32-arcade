#pragma once
#include <cstdint>
#include <cstring>
namespace arcade {
inline bool retryableUpdateHttp(int status) {
  return status == 408 || status == 425 || status == 500 || status == 502 ||
         status == 503 || status == 504 || status == 618;
}
inline const char *updateHttpFailure(int status, int rateRemaining = -1) {
  if (status == 429 || (status == 403 && rateRemaining == 0))
    return "GITHUB RATE LIMIT / RETRY LATER";
  if (status == 618) return "RELEASE LINK EXPIRED / RETRY";
  if (status == 404) return "CHECK REPOSITORY / NO RELEASE";
  if (status == 401) return "GITHUB HTTP 401 / AUTH REQUIRED";
  if (status == 403) return "GITHUB HTTP 403 / ACCESS DENIED";
  if (status == 408 || status == 504) return "GITHUB TIMED OUT / RETRY";
  return "GITHUB SERVER ERROR / RETRY";
}
inline const char *noUpdateMessage(bool upToDate) {
  return upToDate ? "NO NEWER FIRMWARE TO INSTALL" : "CHECK FOR A NEW RELEASE FIRST";
}
inline bool canonicalGithubDownload(const char *url) {
  static constexpr char root[] = "https://github.com/";
  return url && std::strncmp(url,root,sizeof(root)-1) == 0 &&
         std::strstr(url+sizeof(root)-1,"/releases/download/");
}
inline bool githubAssetApiDownload(const char *url) {
  static constexpr char root[] = "https://api.github.com/repos/";
  return url && std::strncmp(url,root,sizeof(root)-1) == 0 &&
         std::strstr(url+sizeof(root)-1,"/releases/assets/");
}
struct Version {
  uint16_t major = 0, minor = 0, patch = 0;
};
inline bool parseVersion(const char *s, Version &v) {
  if (!s)
    return false;
  if (*s == 'v')
    s++;
  uint16_t values[3]{};
  for (int part = 0; part < 3; part++) {
    if (*s < '0' || *s > '9')
      return false;
    uint32_t value = 0;
    int digits = 0;
    while (*s >= '0' && *s <= '9') {
      value = value * 10 + unsigned(*s++ - '0');
      if (value > 65535 || ++digits > 5)
        return false;
    }
    values[part] = uint16_t(value);
    if (part < 2 && *s++ != '.')
      return false;
  }
  if (*s)
    return false;
  v = {values[0], values[1], values[2]};
  return true;
}
inline bool newerVersion(const char *candidate, const char *current) {
  Version a, b;
  if (!parseVersion(candidate, a) || !parseVersion(current, b))
    return false;
  return a.major != b.major   ? a.major > b.major
         : a.minor != b.minor ? a.minor > b.minor
                              : a.patch > b.patch;
}
inline bool validRepository(const char *s) {
  if (!s || !*s || std::strlen(s) > 100)
    return false;
  unsigned slash = 0, segment = 0;
  for (; *s; s++) {
    char c = *s;
    if (c == '/') {
      if (slash++ || !segment)
        return false;
      segment = 0;
    } else {
      if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.'))
        return false;
      segment++;
    }
  }
  return slash == 1 && segment > 0;
}
inline bool parseSha256(const char *s, uint8_t *out) {
  if (!s || std::strlen(s) != 64)
    return false;
  for (int i = 0; i < 32; i++) {
    unsigned value = 0;
    for (int j = 0; j < 2; j++) {
      char c = s[i * 2 + j];
      unsigned n = c >= '0' && c <= '9'   ? c - '0'
                   : c >= 'a' && c <= 'f' ? c - 'a' + 10
                   : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                          : 16;
      if (n == 16)
        return false;
      value = value * 16 + n;
    }
    out[i] = uint8_t(value);
  }
  return true;
}
} // namespace arcade
