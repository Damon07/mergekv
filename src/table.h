#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

namespace mergekv {
class Table {
public:
private:
  std::atomic<uint64_t> merge_idx;
  std::string path;

  std::mutex parts_lock;
};
} // namespace mergekv
