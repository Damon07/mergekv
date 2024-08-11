#pragma once

#include "block_header.h"
#include "bytes_util.h"
#include "inmemory_block.h"
#include "metaindex_row.h"
#include "part.h"
#include "string_util.h"
#include "types.h"
#include <cstddef>
#include <fmt/core.h>
#include <memory>
#include <string_view>

namespace mergekv {

struct PartHeaderJson {
  size_t items_count;
  size_t blocks_count;
  string first_item;
  string last_item;
};

struct PartHeader {
  PartHeader() = default;
  ~PartHeader() = default;

  void Reset() {
    items_count_ = 0;
    blocks_count_ = 0;
    first_item_.clear();
    last_item_.clear();
  }

  string to_string() const {
    return fmt::format("PartHeader: {{items_count: {}, blocks_count: {}, "
                       "first_item: {}, last_item: {}}}",
                       items_count_, blocks_count_,
                       StringUtil::Format(first_item_),
                       StringUtil::Format(last_item_));
  }

  void CopyFrom(const PartHeader &src) {
    items_count_ = src.items_count_;
    blocks_count_ = src.blocks_count_;
    first_item_ = bytes(src.first_item_);
    last_item_ = bytes(src.last_item_);
  }

  void MustReadMetadata(const string &part_path);
  void MustWriteMetadata(const string &part_path);

  size_t items_count_;
  size_t blocks_count_;
  bytes first_item_;
  bytes last_item_;
};

class InMemoryPart {
public:
  InMemoryPart() = default;
  ~InMemoryPart() = default;

  void Reset() {
    ph_.Reset();
    bh_.Reset();
    mr_.Reset();
    metaindex_data_.Reset();
    index_data_.Reset();
    items_data_.Reset();
    lens_data_.Reset();
  }

  void MustStoreToDisk(const string &part_path);
  void Init(InMemoryBlock &ib);
  std::shared_ptr<Part> NewPart();

  PartHeader &ph() { return ph_; }
  BlockHeader &bh() { return bh_; }
  MetaIndexRow &mr() { return mr_; }

  ByteBuffer &metaindex_data() { return metaindex_data_; }
  ByteBuffer &index_data() { return index_data_; }
  ByteBuffer &items_data() { return items_data_; }
  ByteBuffer &lens_data() { return lens_data_; }

private:
  size_t size() const {
    return metaindex_data_.size() + index_data_.size() + items_data_.size() +
           lens_data_.size();
  }

  PartHeader ph_;
  BlockHeader bh_;
  MetaIndexRow mr_;

  ByteBuffer metaindex_data_;
  ByteBuffer index_data_;
  ByteBuffer items_data_;
  ByteBuffer lens_data_;
};

} // namespace mergekv
