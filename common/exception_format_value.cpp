#include "exception_format_value.h"
#include "exception.h"
#include "string_util.h"
#include <cstdint>
#include <fmt/core.h>
#include <fmt/printf.h>

namespace mergekv {
ExceptionFormatValue::ExceptionFormatValue(double dbl_val)
    : type(ExceptionFormatValueType::FORMAT_VALUE_TYPE_DOUBLE),
      dbl_val(dbl_val) {}
ExceptionFormatValue::ExceptionFormatValue(int64_t int_val)
    : type(ExceptionFormatValueType::FORMAT_VALUE_TYPE_INTEGER),
      int_val(int_val) {}
ExceptionFormatValue::ExceptionFormatValue(std::string str_val)
    : type(ExceptionFormatValueType::FORMAT_VALUE_TYPE_STRING),
      str_val(std::move(str_val)) {}

template <>
ExceptionFormatValue ExceptionFormatValue::CreateFormatValue(float value) {
  return ExceptionFormatValue(double(value));
}
template <>
ExceptionFormatValue ExceptionFormatValue::CreateFormatValue(double value) {
  return ExceptionFormatValue(double(value));
}
template <>
ExceptionFormatValue
ExceptionFormatValue::CreateFormatValue(std::string value) {
  return ExceptionFormatValue(std::move(value));
}

template <>
ExceptionFormatValue
ExceptionFormatValue::CreateFormatValue(std::span<uint8_t> value) {
  return ExceptionFormatValue(StringUtil::Format(value));
}

template <>
ExceptionFormatValue
ExceptionFormatValue::CreateFormatValue(std::span<const uint8_t> value) {
  return ExceptionFormatValue(StringUtil::Format(value));
}

template <>
ExceptionFormatValue
ExceptionFormatValue::CreateFormatValue(const char *value) {
  return ExceptionFormatValue(std::string(value));
}
template <>
ExceptionFormatValue ExceptionFormatValue::CreateFormatValue(char *value) {
  return ExceptionFormatValue(std::string(value));
}

std::string
ExceptionFormatValue::Format(const std::string &msg,
                             std::vector<ExceptionFormatValue> &values) {
  try {
    std::vector<fmt::basic_format_arg<fmt::printf_context>> format_args;
    for (auto &val : values) {
      switch (val.type) {
      case ExceptionFormatValueType::FORMAT_VALUE_TYPE_DOUBLE:
        format_args.push_back(
            fmt::detail::make_arg<fmt::printf_context>(val.dbl_val));
        break;
      case ExceptionFormatValueType::FORMAT_VALUE_TYPE_INTEGER:
        format_args.push_back(
            fmt::detail::make_arg<fmt::printf_context>(val.int_val));
        break;
      case ExceptionFormatValueType::FORMAT_VALUE_TYPE_STRING:
        format_args.push_back(
            fmt::detail::make_arg<fmt::printf_context>(val.str_val));
        break;
      }
    }
    return fmt::vsprintf(
        fmt::basic_string_view(msg),
        fmt::basic_format_args<fmt::printf_context>(
            format_args.data(), static_cast<int>(format_args.size())));
  } catch (std::exception &ex) { // LCOV_EXCL_START
    // work-around for oss-fuzz limiting memory which causes issues here
    if (StringUtil::Contains(ex.what(), "fuzz mode")) {
      throw InvalidInputException(msg);
    }
    throw InternalException(
        std::string("Primary exception: ") + msg +
        "\nSecondary exception in ExceptionFormatValue: " + ex.what());
  } // LCOV_EXCL_STOP
}

} // namespace mergekv