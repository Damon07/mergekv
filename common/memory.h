#pragma once
#include "types.h"
#include <cstdint>
#include <mutex>

namespace mergekv {
class CgroupUtil {
public:
  static int AllowedMemory();
  static int RemainingMemory();
  static int GetSystemMemory();
  static int64_t GetMemoryLimit();
  static int64_t GetHierarchicalMemoryLimit();

private:
  static string GrepFirstMatch(const string &data, const string &match,
                               const int index, const string &delimiter);
  static string GetFileContent(const string &stat_name,
                               const string &sysfs_prefix,
                               const string &cgroup_path,
                               const string &cgroup_grep_line);
  static int64_t GetStatGeneric(const string &stat_name,
                                const string &sysfs_prefix,
                                const string &cgroup_path,
                                const string &cgroup_grep_line);
  static int64_t GetMemStat(const string &stat_name);
  static int64_t GetMemStatV2(const string &stat_name);
  static void InitOnce();

  static int allowed_memory;
  static int remaining_memory;
  static int memory_limit;
  static std::once_flag flag;
};

} // namespace mergekv