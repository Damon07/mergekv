#pragma once

#include "encoding_util.h"
#include "exception.h"
#include "inmemory_block.h"
#include "io.h"
#include "types.h"
#include <cstdint>

namespace mergekv {

const uint32_t kMaxIndexBlockSize = 64 * 1024;

struct MetaIndexRow {
  bytes first_item;
  uint32_t bhs_count;
  uint64_t index_block_offset;
  uint32_t index_block_size;

  MetaIndexRow()
      : first_item(), bhs_count(0), index_block_offset(0), index_block_size(0) {
  }

  MetaIndexRow(MetaIndexRow &&) = default;

  void Reset() {
    first_item.clear();
    bhs_count = 0;
    index_block_offset = 0;
    index_block_size = 0;
  }

  void Marshal(bytes &dst) {
    EncodingUtil::MarshalBytes(dst, first_item);
    EncodingUtil::MarshalUint32(dst, bhs_count);
    EncodingUtil::MarshalUint64(dst, index_block_offset);
    EncodingUtil::MarshalUint32(dst, index_block_size);
  }

  bytes_const_span Unmarshal(bytes_const_span src);
  static void UnmarshalMIRows(std::vector<MetaIndexRow> &dst, Reader &r);
};
} // namespace mergekv