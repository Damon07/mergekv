#include "encoding.h"
#include "inmemory_block.h"
#include "string_util.h"
#include "types.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <fmt/core.h>
#include <fmt/printf.h>
#include <gtest/gtest.h>
#include <random>
#include <vector>

namespace mergekv {
auto to_bytes = [](const string &s) -> bytes {
  return bytes(s.begin(), s.end());
};

auto to_string = [](const bytes &b) -> string {
  return string(b.begin(), b.end());
};

auto get_random_bytes = [](std::mt19937 &gen) -> bytes {
  std::uniform_int_distribution<> dis(0, 255);

  auto n = dis(gen) % 256;
  bytes buf(n);
  for (size_t i = 0; i < n; i++) {
    buf[i] = static_cast<uint8_t>(dis(gen));
  }
  return buf;
};

auto get_randown_num = [](std::mt19937 &gen, size_t max) -> size_t {
  std::uniform_int_distribution<> dis(0, max);
  return dis(gen);
};

auto check_marshal_type(MarshalType mt) { return mt >= 0 && mt <= 1; }

TEST(Encoding, CommonPrefixLen) {
  auto f = [](const string &a, const string &b, size_t expect) {
    auto prefix_len = CommonPrefixLen(to_bytes(a), to_bytes(b));
    EXPECT_EQ(prefix_len, expect);
  };

  f("", "", 0);
  f("a", "", 0);
  f("", "a", 0);
  f("a", "a", 1);
  f("abc", "xy", 0);
  f("abc", "abd", 2);
  f("01234567", "01234567", 8);
  f("01234567", "012345678", 8);
  f("012345679", "012345678", 8);
  f("01234569", "012345678", 7);
  f("01234569", "01234568", 7);
}

TEST(InmemoryBlock, Add) {
  std::random_device rd;
  std::mt19937 gen(rd());

  for (size_t i = 0; i < 30; i++) {
    InMemoryBlock block;
    std::vector<string> items;
    size_t total_len = 0;

    for (size_t j = 0; j < i * 100 + 1; j++) {
      auto bd = get_random_bytes(gen);
      if (!block.Add(bd)) {
        break;
      }
      items.push_back(to_string(bd));
      total_len += bd.size();
    }

    EXPECT_EQ(block.items().size(), items.size());
    EXPECT_EQ(block.data().size(), total_len);

    for (size_t j = 0; j < items.size(); j++) {
      auto item = block.items()[j];
      auto data = block.data();
      auto s = item.GetString(data);
      EXPECT_EQ(s, items[j]);
    }
  }
}

TEST(InmemoryBlock, Sort) {
  std::random_device rd;
  std::mt19937 gen(rd());

  for (size_t i = 0; i < 100; i++) {
    InMemoryBlock block;
    std::vector<string> items;
    size_t total_len = 0;

    for (size_t j = 0; j < 1500; j++) {
      auto bd = get_random_bytes(gen);
      if (!block.Add(bd)) {
        break;
      }
      items.push_back(to_string(bd));
      total_len += bd.size();
    }

    block.SortItems();
    std::sort(items.begin(), items.end());

    EXPECT_EQ(block.items().size(), items.size());
    EXPECT_EQ(block.data().size(), total_len);

    for (size_t j = 0; j < items.size(); j++) {
      auto item = block.items()[j];
      auto data = block.data();
      auto s = item.GetString(data);
      EXPECT_EQ(s, items[j]);
    }
  }
}

auto to_hex_string = [](bytes_const_span b) -> string {
  string hex_str = "";
  for (auto byte : b) {
    fmt::format_to(std::back_inserter(hex_str), "0x{:02x},", byte);
  }
  return hex_str;
};

TEST(InmemoryBlock, MarshalUnmarshal) {
  std::random_device rd;
  std::mt19937 gen(rd());

  bytes first_item, common_prefix;
  uint32_t item_len = 0;
  MarshalType mt;

  for (size_t i = 0; i < 1000; i += 10) {
    std::vector<string> items;
    size_t total_len = 0;
    StorageBlock block;
    InMemoryBlock b1, b2;
    first_item.clear();
    common_prefix.clear();

    auto prefix = string("prefix");

    auto items_count = 2 * (get_randown_num(gen, i + 1) + 1);
    for (size_t j = 0; j < items_count / 2; j++) {
      auto tmp = get_random_bytes(gen);
      bytes data(prefix.begin(), prefix.end());
      data.insert(data.end(), tmp.begin(), tmp.end());
      if (!b1.Add(data)) {
        break;
      }

      items.push_back(to_string(data));
      total_len += data.size();
      data = get_random_bytes(gen);
      if (!b1.Add(data)) {
        break;
      }
      items.push_back(to_string(data));
      total_len += data.size();
    }

    std::sort(items.begin(), items.end());
    auto [items_len, _mt] =
        b1.MarshalUnSortedData(block, first_item, common_prefix, 0);
    mt = _mt;
    EXPECT_EQ(items_len, b1.items().size());
    auto first_item_exp = b1.items().front().GetString(b1.data());
    EXPECT_EQ(to_string(first_item), first_item_exp);
    EXPECT_TRUE(check_marshal_type(mt));

    try {
      b2.UnmarshalData(block, first_item, common_prefix, items_len, mt);
    } catch (std::exception &e) {
      FAIL() << "unexpected error when unmarshaling: " << e.what();
    }

    EXPECT_EQ(b2.items().size(), items.size());
    EXPECT_EQ(b2.data().size(), total_len);
    for (size_t j = 0; j < items.size(); j++) {
      auto it2 = b2.items()[j];
      auto item2 = it2.GetString(b2.data());
      EXPECT_EQ(items[j].size(), item2.size());
    }
    for (size_t j = 0; j < b2.items().size(); j++) {
      auto it = b2.items()[j];
      auto item = it.GetString(b2.data());
      if (items[j] != item) {
        FAIL() << "items[" << j << "]=" << items[j]
               << ", but got item=" << item;
      }
    }
  }
}

} // namespace mergekv
