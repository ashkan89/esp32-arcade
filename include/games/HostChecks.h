#pragma once
// Test-only access is compiled out of ESP32 firmware.
#ifdef ARCADE_HOST_TESTS
#include <cstdio>
namespace arcade {
extern unsigned campaignAssertions;
}
#define HOST_CHECK(expr)                                                                           \
  do {                                                                                             \
    ++campaignAssertions;                                                                          \
    if (!(expr)) {                                                                                 \
      std::fprintf(stderr, "Campaign check %s:%d: %s\n", __FILE__, __LINE__, #expr);               \
      return false;                                                                                \
    }                                                                                              \
  } while (0)
#endif
