#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
namespace arcade {
// Canvas folds lowercase to uppercase. The authentication key must match its glyphs.
inline void formatSetupKey(char (&key)[13], uint32_t a, uint32_t b) {
  std::snprintf(key, sizeof(key), "%08lX%04lX", static_cast<unsigned long>(a),
                static_cast<unsigned long>(b & 0xffff));
}
} // namespace arcade
