#include "exception.h"
#include "string_util.h"
#include <exception>
#include <execinfo.h>
#include <string>

namespace mergekv {
Exception::Exception(ExceptionType exception_type, const string &message)
    : std::runtime_error(ToJSON(exception_type, message)) {}

Exception::Exception(ExceptionType exception_type, const string &message,
                     const std::unordered_map<string, string> &extra_info)
    : std::runtime_error(ToJSON(exception_type, message, extra_info)) {}

string Exception::ToJSON(ExceptionType type, const string &message) {
  std::unordered_map<string, string> extra_info;
  return ToJSON(type, message, extra_info);
}

string Exception::ToJSON(ExceptionType type, const string &message,
                         const std::unordered_map<string, string> &extra_info) {
  auto extended_extra_info = extra_info;
  extended_extra_info["stack_trace"] = Exception::GetStackTrace();
  return StringUtil::ToJSONMap(type, message, extended_extra_info);
}

string Exception::GetStackTrace(int max_depth) {
  string result;
  auto callstack = std::unique_ptr<void *[]>(new void *[max_depth]);
  int frames = backtrace(callstack.get(), max_depth);
  char **strs = backtrace_symbols(callstack.get(), frames);
  for (int i = 0; i < frames; i++) {
    result += strs[i];
    result += "\n";
  }
  free(strs);
  return "\n" + result;
}

FatalException::FatalException(ExceptionType type, const string &msg)
    : Exception(type, msg) {}

InternalException::InternalException(const string &msg)
    : Exception(ExceptionType::INTERNAL, msg) {
#ifdef DUCKDB_CRASH_ON_ASSERT
  Printer::Print("ABORT THROWN BY INTERNAL EXCEPTION: " + msg);
  abort();
#endif
}

InvalidInputException::InvalidInputException(const string &msg)
    : Exception(ExceptionType::INVALID_INPUT, msg) {}

InvalidInputException::InvalidInputException(
    const string &msg, const std::unordered_map<string, string> &extra_info)
    : Exception(ExceptionType::INVALID_INPUT, msg, extra_info) {}

IOException::IOException(ExceptionType type, const string &msg)
    : Exception(type, msg) {}

struct ExceptionEntry {
  ExceptionType type;
  char text[48];
};

static constexpr ExceptionEntry EXCEPTION_MAP[] = {
    {ExceptionType::INVALID, "Invalid"},
    {ExceptionType::OUT_OF_RANGE, "Out of Range"},
    {ExceptionType::CONVERSION, "Conversion"},
    {ExceptionType::UNKNOWN_TYPE, "Unknown Type"},
    {ExceptionType::DECIMAL, "Decimal"},
    {ExceptionType::MISMATCH_TYPE, "Mismatch Type"},
    {ExceptionType::DIVIDE_BY_ZERO, "Divide by Zero"},
    {ExceptionType::OBJECT_SIZE, "Object Size"},
    {ExceptionType::INVALID_TYPE, "Invalid type"},
    {ExceptionType::SERIALIZATION, "Serialization"},
    {ExceptionType::TRANSACTION, "TransactionContext"},
    {ExceptionType::NOT_IMPLEMENTED, "Not implemented"},
    {ExceptionType::EXPRESSION, "Expression"},
    {ExceptionType::CATALOG, "Catalog"},
    {ExceptionType::PARSER, "Parser"},
    {ExceptionType::BINDER, "Binder"},
    {ExceptionType::PLANNER, "Planner"},
    {ExceptionType::SCHEDULER, "Scheduler"},
    {ExceptionType::EXECUTOR, "Executor"},
    {ExceptionType::CONSTRAINT, "Constraint"},
    {ExceptionType::INDEX, "Index"},
    {ExceptionType::STAT, "Stat"},
    {ExceptionType::CONNECTION, "Connection"},
    {ExceptionType::SYNTAX, "Syntax"},
    {ExceptionType::SETTINGS, "Settings"},
    {ExceptionType::OPTIMIZER, "Optimizer"},
    {ExceptionType::NULL_POINTER, "NullPointer"},
    {ExceptionType::IO, "IO"},
    {ExceptionType::INTERRUPT, "INTERRUPT"},
    {ExceptionType::FATAL, "FATAL"},
    {ExceptionType::INTERNAL, "INTERNAL"},
    {ExceptionType::INVALID_INPUT, "Invalid Input"},
    {ExceptionType::OUT_OF_MEMORY, "Out of Memory"},
    {ExceptionType::PERMISSION, "Permission"},
    {ExceptionType::PARAMETER_NOT_RESOLVED, "Parameter Not Resolved"},
    {ExceptionType::PARAMETER_NOT_ALLOWED, "Parameter Not Allowed"},
    {ExceptionType::DEPENDENCY, "Dependency"},
    {ExceptionType::MISSING_EXTENSION, "Missing Extension"},
    {ExceptionType::HTTP, "HTTP"},
    {ExceptionType::AUTOLOAD, "Extension Autoloading"},
    {ExceptionType::SEQUENCE, "Sequence"}};

string Exception::ExceptionTypeToString(ExceptionType type) {
  for (auto &e : EXCEPTION_MAP) {
    if (e.type == type) {
      return e.text;
    }
  }
  return "Unknown";
}
string Exception::ConstructMessageRecursive(
    const string &msg, std::vector<ExceptionFormatValue> &values) {
  return ExceptionFormatValue::Format(msg, values);
}

} // namespace mergekv