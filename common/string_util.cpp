#include "string_util.h"
#include <fmt/core.h>

namespace mergekv {
bool StringUtil::Contains(const string &haystack, const string &needle) {
  return (haystack.find(needle) != string::npos);
}

static void WriteJSONValue(const string &value, string &result) {
  result += '"';
  for (auto c : value) {
    // check for characters we need to escape
    switch (c) {
    case '\0':
      result += "\\0";
      break;
    case '\\':
      result += "\\\\";
      break;
    case '\b':
      result += "\\b";
      break;
    case '\f':
      result += "\\f";
      break;
    case '\t':
      result += "\\t";
      break;
    case '\r':
      result += "\\r";
      break;
    case '\n':
      result += "\\n";
      break;
    case '"':
      result += "\\\"";
      break;
    default:
      result += c;
      break;
    }
  }
  result += '"';
}

static void WriteJSONPair(const string &key, const string &value,
                          string &result) {
  WriteJSONValue(key, result);
  result += ":";
  WriteJSONValue(value, result);
}

string StringUtil::ToJSONMap(ExceptionType type, const string &message,
                             const std::unordered_map<string, string> &map) {
  // D_ASSERT(map.find("exception_type") == map.end());
  // D_ASSERT(map.find("exception_message") == map.end());
  string result;
  result += "{";
  // we always write exception type/message
  WriteJSONPair("exception_type", Exception::ExceptionTypeToString(type),
                result);
  result += ",";
  WriteJSONPair("exception_message", message, result);
  for (auto &entry : map) {
    result += ",";
    WriteJSONPair(entry.first, entry.second, result);
  }
  result += "}";
  return result;
}

string StringUtil::Format(std::span<uint8_t> value) {
  string hex_str = "0x";
  for (auto byte : value) {
    fmt::format_to(std::back_inserter(hex_str), "{:02x}", byte);
  }
  return hex_str;
}
string StringUtil::Format(std::span<const uint8_t> value) {
  string hex_str = "0x";
  for (auto byte : value) {
    fmt::format_to(std::back_inserter(hex_str), "{:02x}", byte);
  }
  return hex_str;
}
string StringUtil::Format(const std::vector<uint8_t> &value) {
  string hex_str = "0x";
  for (auto byte : value) {
    fmt::format_to(std::back_inserter(hex_str), "{:02x}", byte);
  }
  return hex_str;
}

std::vector<uint8_t> StringUtil::Bytes(const string &str) {
  std::vector<uint8_t> result(str.begin(), str.end());
  return result;
}

} // namespace mergekv