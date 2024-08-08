#include "string_util.h"
#include "types.h"
#include <cstdint>
#include <fmt/core.h>

namespace mergekv {

const string hextable = "0123456789abcdef";
const string reverseHexTable =
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\xff\xff\xff\xff\xff\xff"
    "\xff\x0a\x0b\x0c\x0d\x0e\x0f\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\x0a\x0b\x0c\x0d\x0e\x0f\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";

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

bytes StringUtil::EncodeHex(bytes_const_span value) {
  bytes dst(value.size() * 2);
  for (auto byte : value) {
    dst.insert(dst.end(), {uint8_t(hextable[(byte >> 4)]),
                           uint8_t(hextable[(byte & 0x0f)])});
  }
  return dst;
}

bytes StringUtil::DecodeHex(bytes_const_span value) {
  if (value.size() % 2 != 0) {
    throw InvalidInputException("Invalid hex string length");
  }
  bytes dst(value.size() / 2);
  for (size_t i = 0; i < value.size(); i += 2) {
    uint8_t high = reverseHexTable[value[i]];
    uint8_t low = reverseHexTable[value[i + 1]];
    if (high > 0x0f) {
      throw InvalidInputException("Invalid hex character: %c", high);
    }

    if (low > 0x0f) {
      throw InvalidInputException("Invalid hex character: %c", low);
    }

    dst.push_back((high << 4) | low);
  }
  return dst;
}

std::vector<uint8_t> StringUtil::Bytes(const string &str) {
  std::vector<uint8_t> result(str.begin(), str.end());
  return result;
}

bytes_const_span StringUtil::BytesConstSpan(const string &str) {
  return bytes_span(reinterpret_cast<uint8_t *>(const_cast<char *>(str.data())),
                    str.size());
}

string_view StringUtil::ToStringView(bytes_const_span value) {
  return string_view(reinterpret_cast<const char *>(value.data()),
                     value.size());
}

string StringUtil::ToString(bytes_const_span value) {
  return string(reinterpret_cast<const char *>(value.data()), value.size());
}

} // namespace mergekv