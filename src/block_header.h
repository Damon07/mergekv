#pragma once

#include "encoding.h"
#include "types.h"
#include <cstdint>
#include <tuple>

namespace mergekv {
struct BlockHeader {
  bytes common_prefix;
  bytes first_item;
  bool no_copy;
  MarshalType mt;
  uint32_t items_count;
  uint64_t items_block_offset;
  uint64_t lens_block_offset;
  uint32_t items_block_size;
  uint32_t lens_block_size;

  BlockHeader()
      : no_copy(false), mt(marshalTypePlain), items_count(0),
        items_block_offset(0), lens_block_offset(0), items_block_size(0),
        lens_block_size(0) {}
  BlockHeader(BlockHeader &&) = default;

  BlockHeader(const BlockHeader &) = delete;
  BlockHeader &operator=(const BlockHeader &) = delete;

  void Reset();

  void Marshal(bytes &dst);
  bytes_const_span UnmarshalNoCopy(bytes_const_span src);
  static void UnmarshalBHNoCopy(std::vector<BlockHeader> &dst,
                                bytes_const_span src, int bh_count);
};
} // namespace mergekv