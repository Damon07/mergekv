#pragma once

#include "string_util.h"
#include "types.h"
#include <cstddef>
#include <fmt/core.h>
#include <string_view>
#include "bytes_util.h"

namespace mergekv {
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

  size_t items_count_;
  size_t blocks_count_;
  bytes first_item_;
  bytes last_item_;
};

class InMemoryPart {
public:


private:
PartHeader ph_;

};

} // namespace mergekv
