#pragma once
#include "types.h"
#include <span>
#include <vector>

namespace mergekv {
enum class ExceptionFormatValueType : uint8_t {
  FORMAT_VALUE_TYPE_DOUBLE,
  FORMAT_VALUE_TYPE_INTEGER,
  FORMAT_VALUE_TYPE_STRING
};

struct ExceptionFormatValue {
  ExceptionFormatValue(double dbl_val);  // NOLINT
  ExceptionFormatValue(int64_t int_val); // NOLINT
  ExceptionFormatValue(string str_val);  // NOLINT

  ExceptionFormatValueType type;

  double dbl_val = 0;
  int64_t int_val = 0;
  string str_val;

public:
  template <class T> static ExceptionFormatValue CreateFormatValue(T value) {
    return int64_t(value);
  }
  static string Format(const string &msg,
                       std::vector<ExceptionFormatValue> &values);
};

template <>
ExceptionFormatValue ExceptionFormatValue::CreateFormatValue(float value);
template <>
ExceptionFormatValue ExceptionFormatValue::CreateFormatValue(double value);
template <>
ExceptionFormatValue ExceptionFormatValue::CreateFormatValue(string value);
template <>
ExceptionFormatValue
ExceptionFormatValue::CreateFormatValue(std::span<uint8_t> value);

template <>
ExceptionFormatValue
ExceptionFormatValue::CreateFormatValue(std::span<const uint8_t> value);

template <>
ExceptionFormatValue ExceptionFormatValue::CreateFormatValue(const char *value);
template <>
ExceptionFormatValue ExceptionFormatValue::CreateFormatValue(char *value);
} // namespace mergekv
