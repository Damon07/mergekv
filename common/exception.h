#pragma once

#include "types.h"
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <unordered_map>

#include "exception_format_value.h"

namespace mergekv {
enum class ExceptionType : uint8_t {
  INVALID = 0,          // invalid type
  OUT_OF_RANGE = 1,     // value out of range error
  CONVERSION = 2,       // conversion/casting error
  UNKNOWN_TYPE = 3,     // unknown type
  DECIMAL = 4,          // decimal related
  MISMATCH_TYPE = 5,    // type mismatch
  DIVIDE_BY_ZERO = 6,   // divide by 0
  OBJECT_SIZE = 7,      // object size exceeded
  INVALID_TYPE = 8,     // incompatible for operation
  SERIALIZATION = 9,    // serialization
  TRANSACTION = 10,     // transaction management
  NOT_IMPLEMENTED = 11, // method not implemented
  EXPRESSION = 12,      // expression parsing
  CATALOG = 13,         // catalog related
  PARSER = 14,          // parser related
  PLANNER = 15,         // planner related
  SCHEDULER = 16,       // scheduler related
  EXECUTOR = 17,        // executor related
  CONSTRAINT = 18,      // constraint related
  INDEX = 19,           // index related
  STAT = 20,            // stat related
  CONNECTION = 21,      // connection related
  SYNTAX = 22,          // syntax related
  SETTINGS = 23,        // settings related
  BINDER = 24,          // binder related
  NETWORK = 25,         // network related
  OPTIMIZER = 26,       // optimizer related
  NULL_POINTER = 27,    // nullptr exception
  IO = 28,              // IO exception
  INTERRUPT = 29,       // interrupt
  FATAL = 30, // Fatal exceptions are non-recoverable, and render the entire DB
              // in an unusable state
  INTERNAL = 31, // Internal exceptions indicate something went wrong internally
                 // (i.e. bug in the code base)
  INVALID_INPUT = 32,          // Input or arguments error
  OUT_OF_MEMORY = 33,          // out of memory
  PERMISSION = 34,             // insufficient permissions
  PARAMETER_NOT_RESOLVED = 35, // parameter types could not be resolved
  PARAMETER_NOT_ALLOWED = 36,  // parameter types not allowed
  DEPENDENCY = 37,             // dependency
  HTTP = 38,
  MISSING_EXTENSION = 39, // Thrown when an extension is used but not loaded
  AUTOLOAD = 40,          // Thrown when an extension is used but not loaded
  SEQUENCE = 41
};

class Exception : public std::runtime_error {
public:
  Exception(ExceptionType exception_type, const string &message);
  Exception(ExceptionType exception_type, const string &message,
            const std::unordered_map<string, string> &extra_info);

public:
  static string ExceptionTypeToString(ExceptionType type);
  static ExceptionType StringToExceptionType(const string &type);

  template <typename... ARGS>
  static string ConstructMessage(const string &msg, ARGS... params) {
    const std::size_t num_args = sizeof...(ARGS);
    if (num_args == 0) {
      return msg;
    }
    std::vector<ExceptionFormatValue> values;
    return ConstructMessageRecursive(msg, values, params...);
  }

  static string ToJSON(ExceptionType type, const string &message);
  static string ToJSON(ExceptionType type, const string &message,
                       const std::unordered_map<string, string> &extra_info);

  static bool InvalidatesTransaction(ExceptionType exception_type);
  static bool InvalidatesDatabase(ExceptionType exception_type);

  static string
  ConstructMessageRecursive(const string &msg,
                            std::vector<ExceptionFormatValue> &values);

  template <class T, typename... ARGS>
  static string
  ConstructMessageRecursive(const string &msg,
                            std::vector<ExceptionFormatValue> &values, T param,
                            ARGS... params) {
    values.push_back(ExceptionFormatValue::CreateFormatValue<T>(param));
    return ConstructMessageRecursive(msg, values, params...);
  }

  static bool UncaughtException();

  static string GetStackTrace(int max_depth = 120);
  static string FormatStackTrace(const string &message = "") {
    return (message + "\n" + GetStackTrace());
  }
};

class FatalException : public Exception {
public:
  explicit FatalException(const string &msg)
      : FatalException(ExceptionType::FATAL, msg) {}
  template <typename... ARGS>
  explicit FatalException(const string &msg, ARGS... params)
      : FatalException(ConstructMessage(msg, params...)) {}

protected:
  explicit FatalException(ExceptionType type, const string &msg);
  template <typename... ARGS>
  explicit FatalException(ExceptionType type, const string &msg, ARGS... params)
      : FatalException(type, ConstructMessage(msg, params...)) {}
};

class InternalException : public Exception {
public:
  explicit InternalException(const string &msg);

  template <typename... ARGS>
  explicit InternalException(const string &msg, ARGS... params)
      : InternalException(ConstructMessage(msg, params...)) {}
};

class InvalidInputException : public Exception {
public:
  explicit InvalidInputException(const string &msg);
  explicit InvalidInputException(
      const string &msg, const std::unordered_map<string, string> &extra_info);

  template <typename... ARGS>
  explicit InvalidInputException(const string &msg, ARGS... params)
      : InvalidInputException(ConstructMessage(msg, params...)) {}
};

class IOException : public Exception {
public:
  explicit IOException(const string &msg)
      : IOException(ExceptionType::IO, msg) {}

  template <typename... ARGS>
  explicit IOException(const string &msg, ARGS... params)
      : IOException(ConstructMessage(msg, params...)) {}

protected:
  explicit IOException(ExceptionType type, const string &msg);
  template <typename... ARGS>
  explicit IOException(ExceptionType type, const string &msg, ARGS... params)
      : IOException(type, ConstructMessage(msg, params...)) {}
};

} // namespace mergekv