#pragma once

#include "exception.h"
#include <string>
#include <unordered_map>

namespace mergekv {
/**
 * String Utility Functions
 * Note that these are not the most efficient implementations (i.e., they copy
 * memory) and therefore they should only be used for debug messages and other
 * such things.
 */
class StringUtil {
public:
  static bool Contains(const std::string &haystack, const std::string &needle);
  static std::string
  ToJSONMap(ExceptionType type, const std::string &message,
            const std::unordered_map<std::string, std::string> &map);
  static std::string Format(std::span<uint8_t> value);
  static std::string Format(std::vector<uint8_t> value);
  static std::string Format(std::span<const uint8_t> value);
  static std::vector<uint8_t> Bytes(const std::string &str);
};
} // namespace mergekv
