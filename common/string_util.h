#pragma once

#include "exception.h"
#include "types.h"
#include <unordered_map>
#include <vector>

namespace mergekv {
/**
 * String Utility Functions
 * Note that these are not the most efficient implementations (i.e., they copy
 * memory) and therefore they should only be used for debug messages and other
 * such things.
 */
class StringUtil {
public:
  static bool Contains(const string &haystack, const string &needle);
  static string ToJSONMap(ExceptionType type, const string &message,
                          const std::unordered_map<string, string> &map);
  static string Format(std::span<uint8_t> value);
  static string Format(const std::vector<uint8_t> &value);
  static string Format(std::span<const uint8_t> value);
  static std::vector<uint8_t> Bytes(const string &str);
  static bytes_const_span BytesConstSpan(const string &str);
  static bytes_const_span BytesConstSpan(string_view str);
  static string_view ToStringView(bytes_const_span value);
  static string ToString(bytes_const_span value);
  static bytes EncodeHex(bytes_const_span value);
  static bytes DecodeHex(bytes_const_span value);
  static std::vector<string> Split(const string &s, const string &delim);
  static string ltrim_space(const string &s);
  static string rtrim_space(const string &s);
  static string trim_space(const string &s);
};
} // namespace mergekv
