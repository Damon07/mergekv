#include "memory.h"
#include "exception.h"
#include "string_util.h"
#include <fstream>
#include <mutex>

#define OS_DARWIN 1

#ifdef OS_LINUX
#include <sys/sysinfo.h>
#endif

#ifdef OS_DARWIN
#include <sys/sysctl.h>
#endif

namespace mergekv {

const int kMaxInt = static_cast<int>(~0U >> 1);
const double kAllowedPercent = 60;

std::once_flag CgroupUtil::flag = std::once_flag();

int CgroupUtil::GetSystemMemory() {
#ifdef OS_LINUX
  struct sysinfo info;
  if (sysinfo(&info) != 0) {
    throw IOException("cannot get system memory");
  }
  auto total = kMaxInt;
  if (uint64_t(total) / uint64_t(info.totalram) > uint64_t(info.unit)) {
    total = int(uint64_t(info.totalram) * uint64_t(info.unit));
  }

  auto mem = GetMemoryLimit();
  if (mem <= 0 || int64_t(int(mem)) != mem || int(mem) > total) {
    mem = GetHierarchicalMemoryLimit();
    if (mem <= 0 || int64_t(int(mem)) != mem || int(mem) > total) {
      return total;
    }
  }

  return int(mem);
#endif

#ifdef OS_DARWIN
  int mib[2];
  mib[0] = CTL_HW;
  mib[1] = HW_MEMSIZE;
  int size = 0;
  size_t len = sizeof(size);
  if (sysctl(mib, 2, &size, &len, nullptr, 0) != 0) {
    throw FatalException("cannot get system memory");
  }
  return size;
#endif

#if !defined(OS_DARWIN) && !defined(OS_LINUX)
  throw FatalException("unsupported platform");
#endif
}

int64_t CgroupUtil::GetMemoryLimit() {
  int64_t limit = 0;
  try {
    limit = GetMemStat("memory.limit_in_bytes");
  } catch (...) {
    try {
      limit = GetMemStatV2("memory.max");
    } catch (...) {
      return 0;
    }
  }
  return limit;
}

int64_t CgroupUtil::GetHierarchicalMemoryLimit() {
  try {
    auto data = GetFileContent("memory.stat", "/sys/fs/cgroup/memory",
                               "/proc/self/cgroup", "memory");
    auto mem_stat = GrepFirstMatch(data, "hierarchical_memory_limit", 1, " ");
    return std::stoll(mem_stat);
  } catch (...) {
    return 0;
  }
}

int64_t CgroupUtil::GetStatGeneric(const string &stat_name,
                                   const string &sysfs_prefix,
                                   const string &cgroup_path,
                                   const string &cgroup_grep_line) {
  auto data =
      GetFileContent(stat_name, sysfs_prefix, cgroup_path, cgroup_grep_line);
  data = StringUtil::trim_space(data);
  return std::stoll(data);
}

string CgroupUtil::GetFileContent(const string &stat_name,
                                  const string &sysfs_prefix,
                                  const string &cgroup_path,
                                  const string &cgroup_grep_line) {
  string path = sysfs_prefix + "/" + stat_name;
  std::ifstream file(path);
  if (!file.is_open()) {
    throw IOException("cannot open file: %s", path);
  }
  string cgroup_data;
  file >> cgroup_data;
  auto sub_path = GrepFirstMatch(cgroup_data, stat_name, 0, cgroup_grep_line);
  string file_path = sysfs_prefix + "/" + sub_path + "/" + stat_name;
  std::ifstream sub_file(file_path);
  if (!sub_file.is_open()) {
    throw IOException("cannot open file: %s", file_path);
  }
  string data;
  sub_file >> data;
  return data;
}

string CgroupUtil::GrepFirstMatch(const string &data, const string &match,
                                  const int index, const string &delimiter) {
  auto lines = StringUtil::Split(data, "\n");
  for (auto &line : lines) {
    if (!StringUtil::Contains(line, delimiter)) {
      continue;
    }
    auto parts = StringUtil::Split(line, delimiter);
    if (index < parts.size()) {
      StringUtil::trim_space(parts[index]);
    }
  }
  throw InvalidInputException("cannot find %s in %s", match, data);
}

int64_t CgroupUtil::GetMemStat(const string &stat_name) {
  return GetStatGeneric(stat_name, "/sys/fs/cgroup", "/proc/self/cgroup", "");
}

int64_t CgroupUtil::GetMemStatV2(const string &stat_name) {
  return GetStatGeneric(stat_name, "/sys/fs/cgroup/memory", "/proc/self/cgroup",
                        "memory");
}

void CgroupUtil::InitOnce() {
  memory_limit = GetSystemMemory();
  auto percent = kAllowedPercent / 100.0;
  allowed_memory = int(memory_limit * percent);
  remaining_memory = memory_limit - allowed_memory;
}

int CgroupUtil::AllowedMemory() {
  std::call_once(flag, InitOnce);
  return allowed_memory;
}

int CgroupUtil::RemainingMemory() {
  std::call_once(flag, InitOnce);
  return remaining_memory;
}

} // namespace mergekv