#include "inmemory_block.h"
#include "encoding.h"
#include "encoding_util.h"
#include "exception.h"
#include "types.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fmt/core.h>
#include <fmt/printf.h>
#include <vector>

namespace mergekv {

void InMemoryBlock::CopyFrom(InMemoryBlock &src) {
  common_prefix_ = bytes(src.common_prefix_.begin(), src.common_prefix_.end());
  data_ = bytes(src.data_.begin(), src.data_.end());
  items_ = std::vector<Item>(src.items_.begin(), src.items_.end());
}

auto printAscii = [](const string_view str) {
  string asciiRepresentation;
  for (char c : str) {
    asciiRepresentation += fmt::format("{} ", static_cast<int>(c));
  }
  return asciiRepresentation;
};

bool InMemoryBlock::CompareItems(Item a, Item b) const {
  auto cLen = common_prefix_.size();
  a.start += cLen;
  b.start += cLen;
  return a.GetString(data_) < b.GetString(data_);
}

void InMemoryBlock::SortItems() {
  if (!IsSorted()) {
    UpdateCommonPrefixUnsorted();
    // sort items_
    std::sort(items_.begin(), items_.end(),
              [this](Item a, Item b) { return CompareItems(a, b); });
    return;
  }
  UpdateCommonPrefixSorted();
}

void InMemoryBlock::UpdateCommonPrefixSorted() {
  if (items_.size() <= 1) {
    common_prefix_.clear();
    return;
  }

  auto cp = items_.front().GetBytes(data_);
  auto cpLen = CommonPrefixLen(cp, items_.back().GetBytes(data_));
  common_prefix_ = bytes(cp.begin(), cp.begin() + cpLen);
}

void InMemoryBlock::UpdateCommonPrefixUnsorted() {
  common_prefix_.clear();
  if (items_.empty()) {
    return;
  }

  auto cp = items_.front().GetBytes(data_);
  for (size_t i = 1; i < items_.size(); i++) {
    auto item_bytes = items_[i].GetBytes(data_);
    if (HasPrefix(item_bytes, cp)) {
      continue;
    }
    auto cpLen = CommonPrefixLen(item_bytes, cp);
    if (cpLen == 0) {
      return;
    }

    cp = cp.subspan(0, cpLen);
  }
  common_prefix_.insert(common_prefix_.begin(), cp.begin(), cp.end());
}

bool InMemoryBlock::Add(bytes &data) {
  if (data_.size() + data.size() > kMaxInmemoryBlockSize) {
    return false;
  }
  if (data_.capacity() == 0) {
    data_.reserve(kMaxInmemoryBlockSize);
    items_.reserve(512);
  }

  auto data_len = data_.size();
  data_.insert(data_.end(), data.begin(), data.end());
  items_.emplace_back(data_len, data_.size());
  return true;
}

bool InMemoryBlock::IsSorted() const {
  return std::is_sorted(items_.begin(), items_.end(),
                        [this](Item a, Item b) { return CompareItems(a, b); });
}

InMemoryBlock::marshal_results
InMemoryBlock::MarshalUnSortedData(StorageBlock &block, bytes &first_item_dst,
                                   bytes &common_prefix_dst,
                                   int compress_level) {
  SortItems();
  return MarshalData(block, first_item_dst, common_prefix_dst, compress_level);
}

InMemoryBlock::marshal_results
InMemoryBlock::MarshalSortedData(StorageBlock &block, bytes &first_item_dst,
                                 bytes &common_prefix_dst, int compress_level) {
  if (!IsSorted()) {
    throw FatalException("MarshalSortedData: items are not sorted");
  }
  UpdateCommonPrefixSorted();
  return MarshalData(block, first_item_dst, common_prefix_dst, compress_level);
}

InMemoryBlock::marshal_results
InMemoryBlock::MarshalData(StorageBlock &block, bytes &first_item_dst,
                           bytes &common_prefix_dst, int compress_level) {
  if (items_.empty()) {
    throw FatalException("MarshalData: items is empty");
  }
  if (items_.size() >= uint64_t(1) << 32) {
    throw FatalException("BUG: the number of items in the block must be "
                         "smaller than %d; got %d items",
                         uint64_t(1) << 32, items_.size());
  }

  auto first_item = items_.front().GetBytes(data_);
  first_item_dst.insert(first_item_dst.end(), first_item.begin(),
                        first_item.end());
  common_prefix_dst.insert(common_prefix_dst.end(), common_prefix_.begin(),
                           common_prefix_.end());
  // most of the items are prefixed by common_prefix_ or items are few
  if (data_.size() - common_prefix_.size() * items_.size() < 64 ||
      items_.size() < 2) {
    // Use plain encoding for small block, since it is cheaper.
    MarshalDataPlain(block);
    return {uint32_t(items_.size()), marshalTypePlain};
  }

  // marshal items data.
  auto cp_len = common_prefix_.size();
  auto pre_item = first_item.subspan(cp_len);
  auto pre_prefix_len = uint64_t(0);
  // todo: reserve items_bytes size
  auto items_bytes = bytes();
  auto x_lens = u64s();
  x_lens.reserve(items_.size() - 1);
  // calculate the prefix lens and save the suffixes
  for (size_t i = 1; i < items_.size(); i++) {
    auto it = items_[i]; // copy, since we will change it
    it.start += cp_len;
    auto item = it.GetBytes(data_);
    auto prefix_len = CommonPrefixLen(pre_item, item);
    // save suffix
    items_bytes.insert(items_bytes.end(), item.begin() + prefix_len,
                       item.end());
    // make x_len smaller
    auto x_len = prefix_len ^ pre_prefix_len;
    pre_item = item;
    pre_prefix_len = prefix_len;
    // save prefix len
    x_lens.push_back(x_len);
  }

  // todo: reserve b_lens size
  bytes lens_bytes;
  EncodingUtil::MarshalVarUint64s(lens_bytes, x_lens);

  block.items_data->clear();
  EncodingUtil::CompressZSTDLevel(*block.items_data, items_bytes,
                                  compress_level);
  // for reuse
  x_lens.clear();
  // save item lens
  auto pre_item_len = uint64_t(first_item.size() - cp_len);
  for (size_t i = 1; i < items_.size(); i++) {
    auto it = items_[i];
    auto item_len = uint64_t(it.end - it.start - cp_len);
    auto x_len = item_len ^ pre_item_len;
    pre_item_len = item_len;
    x_lens.push_back(x_len);
  }

  EncodingUtil::MarshalVarUint64s(lens_bytes, x_lens);

  block.lens_data->clear();
  EncodingUtil::CompressZSTDLevel(*block.lens_data, lens_bytes, compress_level);
  if (double(block.items_data->size()) >
      0.9 * double(data_.size() - common_prefix_.size() * items_.size())) {
    // Bad compression rate. It is cheaper to use plain encoding.
    MarshalDataPlain(block);
    return {uint32_t(items_.size()), marshalTypePlain};
  }

  return {uint32_t(items_.size()), marshalTypeSZTD};
}

void InMemoryBlock::MarshalDataPlain(StorageBlock &block) {
  // Marshal items data.
  // There is no need in marshaling the first item, since it is returned
  // to the caller in marshalData.
  auto cp_len = common_prefix_.size();
  block.items_data->clear();
  auto &items_data = block.items_data;
  for (size_t i = 1; i < items_.size(); i++) {
    auto it = items_[i]; // copy, since we will change it
    it.start += cp_len;
    auto item = it.GetBytes(data_);
    items_data->insert(items_data->end(), item.begin(), item.end());
  }

  // Marshal lens data.
  block.lens_data->clear();
  auto &lens_data = block.lens_data;
  for (size_t i = 1; i < items_.size(); i++) {
    EncodingUtil::MarshalUint64(
        *lens_data, uint64_t(items_[i].end - items_[i].start - cp_len));
  }
}

void InMemoryBlock::UnmarshalData(const StorageBlock &block,
                                  bytes_const_span first_item,
                                  bytes_const_span common_prefix,
                                  uint32_t items_count, MarshalType mt) {
  if (items_count == 0) {
    throw FatalException("UnmarshalData: items_count is 0");
  }

  common_prefix_.insert(common_prefix_.begin(), common_prefix.begin(),
                        common_prefix.end());
  switch (mt) {
  case marshalTypePlain: {
    UnmarshalDataPlain(block, first_item, items_count);
    if (!IsSorted()) {
      InvalidInputException(
          "plain data block contains unsorted items; items:\n%s",
          debugItemString());
    }
    return;
  }
  case marshalTypeSZTD: {
    break;
  }
  default:
    throw InvalidInputException("UnmarshalData: unknown marshal type %d", mt);
  }

  // unmarshal marshalTypeSZTD data
  // todo, reserve buf_data size
  bytes buf_data;
  EncodingUtil::DecompressZSTD(buf_data, *block.lens_data);

  u64s lens;
  lens.resize(items_count * 2);

  auto prefix_lens = u64s_span(lens.begin(), lens.begin() + items_count);
  auto item_lens = u64s_span(lens.begin() + items_count, lens.end());

  // unmarshal prefix lens
  u64s dst;
  dst.resize(items_count - 1);
  auto lens_data_tail = EncodingUtil::UnmarshalVarUint64s(dst, buf_data);

  prefix_lens[0] = 0;
  size_t idx = 0;
  for (auto &x_len : dst) {
    prefix_lens[idx + 1] = x_len ^ prefix_lens[idx];
    ++idx;
  }

  // unmarshal items lens
  lens_data_tail = EncodingUtil::UnmarshalVarUint64s(dst, lens_data_tail);
  if (!lens_data_tail.empty()) {
    throw InvalidInputException(
        "unexpected tail left unmarshaling %d lens; tail size=%d; contents=%X",
        items_count, lens_data_tail.size(), lens_data_tail);
  }

  item_lens[0] = first_item.size() - common_prefix.size();
  auto data_len = common_prefix.size() * items_count + item_lens[0];
  idx = 0;
  for (auto &x_len : dst) {
    auto item_len = x_len ^ item_lens[idx];
    item_lens[idx + 1] = item_len;
    data_len += item_len;
    ++idx;
  }

  // then we can unmarshal items use lens
  buf_data.resize(0);
  EncodingUtil::DecompressZSTD(buf_data, *(block.items_data));
  data_.reserve(data_len);
  data_.resize(0);
  data_.insert(data_.end(), first_item.begin(), first_item.end());
  items_.resize(items_count);
  items_[0] = Item(0, uint32_t(first_item.size()));
  auto pre_item = bytes_const_span(data_.begin() + common_prefix.size(),
                                   first_item.size() - common_prefix.size());
  bytes_const_span bs = buf_data;
  for (size_t i = 1; i < items_count; i++) {
    auto item_len = item_lens[i];
    auto prefix_len = prefix_lens[i];
    if (prefix_len > item_len) {
      throw InvalidInputException("prefix_len %d is larger than item_len %d",
                                  prefix_len, item_len);
    }
    auto suffix_len = item_len - prefix_len;
    if (bs.size() < suffix_len) {
      throw InvalidInputException(
          "not enought data unmarshaling %d item; suffix_len=%d; "
          "bs.size=%d",
          i, suffix_len, bs.size());
    }
    auto data_start = data_.size();
    data_.insert(data_.end(), common_prefix.begin(), common_prefix.end());
    data_.insert(data_.end(), pre_item.begin(), pre_item.begin() + prefix_len);
    data_.insert(data_.end(), bs.begin(), bs.begin() + suffix_len);
    items_[i] = Item(uint32_t(data_start), uint32_t(data_.size()));
    bs = bs.subspan(suffix_len);
    pre_item =
        bytes_const_span(data_.begin() + data_.size() - item_len, data_.end());
  }

  if (!bs.empty()) {
    throw InvalidInputException(
        "unexpected tail left after itemsData with len %d: %s", bs.size(), bs);
  }

  if (data_.size() != data_len) {
    throw InvalidInputException(
        "unexpected data len after unmarshaling %d items; expected %d; got %d",
        items_count, data_len, data_.size());
  }

  if (!IsSorted()) {
    InvalidInputException(
        "decoded data block contains unsorted items; items:\n%s",
        debugItemString());
  }
}

void InMemoryBlock::UnmarshalDataPlain(const StorageBlock &block,
                                       bytes_const_span first_item,
                                       uint32_t items_count) {
  u64s lens_buf;
  lens_buf.resize(items_count);
  lens_buf[0] = first_item.size() - common_prefix_.size();
  bytes_const_span b = *block.lens_data;
  for (size_t i = 1; i < items_count; i++) {
    if (b.size() < 8) {
      throw InvalidInputException(
          "too short tail for decoding len from lensData; got %d bytes; want "
          "at least %d bytes",
          b.size(), 8);
    }
    auto i_len = EncodingUtil::UnmarshalUint64(b);
    b = b.subspan(8);
    lens_buf[i] = i_len;
  }

  if (!b.empty()) {
    throw InvalidInputException(
        "unexpected tail left after lensData with len %d: %s", b.size(), b);
  }

  // Unmarshal items data.
  auto data_len = first_item.size() + block.items_data->size() +
                  common_prefix_.size() * (items_count - 1);
  data_.reserve(data_len);
  data_.resize(0);
  data_.insert(data_.end(), first_item.begin(), first_item.end());
  items_.resize(items_count);
  items_[0] = Item(0, uint32_t(data_.size()));
  bytes_const_span bs = *block.items_data;
  for (size_t i = 1; i < items_count; i++) {
    auto item_len = lens_buf[i];
    if (bs.size() < item_len) {
      throw InvalidInputException(
          "not enought data unmarshaling %d item; item_len=%d; bs.size=%d", i,
          item_len, bs.size());
    }
    auto data_start = data_.size();
    data_.insert(data_.end(), common_prefix_.begin(), common_prefix_.end());
    data_.insert(data_.end(), bs.begin(), bs.begin() + item_len);
    items_[i] = Item(uint32_t(data_start), uint32_t(data_.size()));
    bs = bs.subspan(item_len);
  }
  if (!bs.empty()) {
    throw InvalidInputException(
        "unexpected tail left after itemsData with len %d: %s", bs.size(), bs);
  }
  if (data_.size() != data_len) {
    throw InvalidInputException("unexpected data len after unmarshaling plain "
                                "%d items; expected %d; got %d",
                                items_count, data_len, data_.size());
  }
}

string InMemoryBlock::debugItemString() const {
  string s, pre_item;
  size_t idx = 0;
  for (auto &it : items_) {
    auto item = it.GetString(data_);
    if (item < pre_item) {
      s += fmt::format("!!! the next item is smaller than the previous item "
                       "!!! items[{}]: {} < {}\n",
                       idx, item, pre_item);
    }
  }
  return s;
}

} // namespace mergekv