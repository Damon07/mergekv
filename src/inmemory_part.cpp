#include "inmemory_part.h"
#include "bytes_util.h"
#include "encoding_util.h"
#include "exception.h"
#include "file.h"
#include "filenames.h"
#include "inmemory_block.h"
#include "metaindex_row.h"
#include "types.h"
#include <cstddef>

namespace mergekv {

void InMemoryPart::MustStoreToDisk(const string &part_path) {
  fs::path base_path = part_path;
  fs::create_directories(base_path);

  ph_.MustWriteMetadata(part_path);

  auto metaindex_path = fs::path(base_path / kMetaindexFilename);
  auto index_path = fs::path(base_path / kIndexFilename);
  auto items_path = fs::path(base_path / kItemsFilename);
  auto lens_path = fs::path(base_path / kLensFilename);

  FileUtils::MustWriteSync(metaindex_path, *(metaindex_data_.data()));
  FileUtils::MustWriteSync(index_path, *index_data_.data());
  FileUtils::MustWriteSync(items_path, *items_data_.data());
  FileUtils::MustWriteSync(lens_path, *lens_data_.data());
}

void InMemoryPart::Init(InMemoryBlock &ib) {
  Reset();
  auto sb = StorageBlock(lens_data_.data(), items_data_.data());
  int compress_level = -5;
  auto [item_count, mt] = ib.MarshalUnSortedData(
      sb, bh_.first_item, bh_.common_prefix, compress_level);
  bh_.items_count = item_count;
  bh_.mt = mt;

  ph_.items_count_ = ib.items().size();
  ph_.blocks_count_ = 1;
  auto first_item = ib.items().front().GetString(ib.data());
  ph_.first_item_.insert(ph_.first_item_.begin(), first_item.begin(),
                         first_item.end());
  auto last_item = ib.items().back().GetString(ib.data());
  ph_.last_item_.insert(ph_.last_item_.begin(), last_item.begin(),
                        last_item.end());
  bh_.items_block_offset = 0;
  bh_.items_block_size = items_data_.size();
  bh_.lens_block_offset = 0;
  bh_.lens_block_size = lens_data_.size();

  bytes buf;
  bh_.Marshal(buf);
  if (buf.size() != kMaxIndexBlockSize * 3) {
    throw FatalException(
        "Too big block header size: %d, can not exceed %d bytes", buf.size(),
        kMaxIndexBlockSize * 3);
  }
  auto index_data = *index_data_.data();
  EncodingUtil::CompressZSTDLevel(index_data, buf, compress_level);

  mr_.first_item.insert(mr_.first_item.begin(), bh_.first_item.begin(),
                        bh_.first_item.end());
  mr_.bhs_count = 1;
  mr_.index_block_offset = 0;
  mr_.index_block_size = index_data.size();
  buf.resize(0);
  mr_.Marshal(buf);
  EncodingUtil::CompressZSTDLevel(*metaindex_data_.data(), buf, compress_level);
}
} // namespace mergekv