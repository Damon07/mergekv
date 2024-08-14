#pragma once

#include "file.h"
#include "metaindex_row.h"
#include "part_header.h"
#include "types.h"
#include <cstddef>
#include <memory>
#include <vector>

namespace mergekv {
class Part {

private:
  PartHeader ph_;
  string part_path_;
  size_t size_;
  std::vector<MetaIndexRow> mrs_;

  std::unique_ptr<BufferFileWriter> metaindex_data_;
};
} // namespace mergekv
