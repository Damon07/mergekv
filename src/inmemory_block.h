#pragma once

#include "encoding.h"
#include "types.h"
#include <cstdint>
#include <span>
#include <string_view>
#include <tuple>
#include <vector>
namespace mergekv {

const int kMaxInmemoryBlockSize = 64 * 1024;

struct Item {
  Item() = default;
  Item(uint32_t start, uint32_t end) : start(start), end(end) {}
  std::span<uint8_t> GetBytes(bytes &data) const {
    return std::span<uint8_t>(data.begin() + start, data.begin() + end);
  }

  // bytes_const_span GetBytes(bytes_const_span &data) const {
  //   return bytes_const_span(data.begin() + start, data.begin() + end);
  // }

  std::string_view GetString(const bytes &data) const {
    return std::string_view(reinterpret_cast<const char *>(data.data() + start),
                            end - start);
  }

  std::string_view GetString(const bytes_const_span &data) const {
    return std::string_view(reinterpret_cast<const char *>(data.data() + start),
                            end - start);
  }

  uint32_t start;
  uint32_t end;
};

struct StorageBlock {
  bytes lens_data;
  bytes items_data;
};

class InMemoryBlock {
public:
  using marshal_results = std::tuple<uint32_t, MarshalType>;
  InMemoryBlock() = default;
  void CopyFrom(InMemoryBlock &src);
  void SortItems();
  int GetSizeBytes() const;
  bool Add(bytes &data);
  marshal_results MarshalUnSortedData(StorageBlock &block,
                                      bytes &first_item_dst,
                                      bytes &common_prefix_dst,
                                      int compress_level);
  marshal_results MarshalSortedData(StorageBlock &block, bytes &first_item_dst,
                                    bytes &common_prefix_dst,
                                    int compress_level);
  void UnmarshalData(const StorageBlock &block, bytes_const_span first_item,
                     bytes_const_span common_prefix, uint32_t items_count,
                     MarshalType mt);

  std::span<const Item> items() const { return items_; }

  bytes_const_span data() const { return data_; }
  // for testing
  void set_data(bytes &data) { data_ = bytes(data.begin(), data.end()); }
  void set_items(std::vector<Item> &items) {
    items_ = std::vector<Item>(items.begin(), items.end());
  }

private:
  void UpdateCommonPrefixSorted();
  void UpdateCommonPrefixUnsorted();
  bool IsSorted() const;
  marshal_results MarshalData(StorageBlock &block, bytes &first_item_dst,
                              bytes &common_prefix_dst, int compress_level);

  std::string debugItemString() const;
  bool CompareItems(Item a, Item b) const;
  void MarshalDataPlain(StorageBlock &block);
  void UnmarshalDataPlain(const StorageBlock &block,
                          bytes_const_span first_item, uint32_t items_count);

private:
  bytes common_prefix_;
  bytes data_;
  std::vector<Item> items_;
};

} // namespace mergekv
