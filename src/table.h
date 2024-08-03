#pragma once

#include "types.h"
#include <atomic>
#include <cstdint>
#include <mutex>

namespace mergekv {
class Table {
public:
private:
  std::atomic<uint64_t> merge_idx;
  string path;

  std::mutex parts_lock;
};
} // namespace mergekv
