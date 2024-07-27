#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>

namespace mergekv {

class RawItemsShardNoPad {
public:
private:
  std::atomic<uint64_t> flush_deadline_ms;
  std::mutex lock;
};

class RawItemsShard {
public:
private:
};

class RawItemsShards {
public:
private:
  std::atomic<uint64_t> flush_deadline_ms;
  std::atomic<uint32_t> shard_idx;
};

} // namespace mergekv