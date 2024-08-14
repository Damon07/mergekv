#pragma once

#include "block_header.h"
#include "bytes_util.h"
#include "inmemory_block.h"
#include "metaindex_row.h"
#include "part.h"
#include "part_header.h"
#include "string_util.h"
#include "types.h"
#include <cstddef>
#include <fmt/core.h>
#include <memory>
#include <string_view>

namespace mergekv {

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
