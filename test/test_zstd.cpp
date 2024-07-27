#include "encoding_util.h"
#include "types.h"
#include <fmt/core.h>
#include <fmt/format.h>
#include <gtest/gtest.h>
#include <iterator>
namespace mergekv {

bytes decodeHex(const std::string &hex) {
  bytes bytes;

  for (size_t i = 0; i < hex.length(); i += 2) {
    std::string byteString = hex.substr(i, 2);
    uint8_t byte = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
    bytes.push_back(byte);
  }

  return bytes;
}

void testCompressLevel(bytes &src, int level) {
  bytes dst_c, dst_d;
  EncodingUtil::CompressZSTDLevel(dst_c, src, level);
  EncodingUtil::DecompressZSTD(dst_d, dst_c);
  EXPECT_EQ(src, dst_d);
}

TEST(test_compress_level, test_name) {
  std::string ss = "foobar baz";
  bytes src(ss.begin(), ss.end());

  for (int level = 1; level <= 22; level++) {
    testCompressLevel(src, level);
  }

  // Test invalid compression levels - they should clamp
  // to the closest valid levels.
  testCompressLevel(src, -123);
  testCompressLevel(src, 234324);
}

std::string c_block_hex("28B52FFD00007D000038C0A907DFD40300015407022B0E02");
std::string d_block_hex_expected(
    "C0A907DFD4030000000000000000000000000000000000000000000000"
    "00000000000000000000000000000000000000000000000000000000000000000000000"
    "00000000000000000000000000000000000000000000000000000000000000000000000"
    "00000000000000000000000000000000000000000000000000000000000000000000000"
    "000000000000000000000000000000000");

TEST(TestDecompressSmallBlock, empty_dst_buff) {

  auto c_block = decodeHex(c_block_hex);
  auto d_block_expected = decodeHex(d_block_hex_expected);
  bytes buf;
  EncodingUtil::DecompressZSTD(buf, c_block);
  EXPECT_EQ(buf, d_block_expected);
}

TEST(TestDecompressSmallBlock, small_dst_buff) {

  auto c_block = decodeHex(c_block_hex);
  auto d_block_expected = decodeHex(d_block_hex_expected);
  bytes buf;
  buf.reserve(d_block_expected.size() / 2);
  EncodingUtil::DecompressZSTD(buf, c_block);
  EXPECT_EQ(buf, d_block_expected);
}

TEST(TestDecompressSmallBlock, enough_dst_buff) {
  auto c_block = decodeHex(c_block_hex);
  auto d_block_expected = decodeHex(d_block_hex_expected);
  bytes buf;
  buf.reserve(d_block_expected.size());
  EncodingUtil::DecompressZSTD(buf, c_block);
  EXPECT_EQ(buf, d_block_expected);
}

TEST(TestCompressDecompress, multi_frames) {
  fmt::memory_buffer bb;
  while (bb.size() < 3 * 12 * 1024) {
    fmt::format_to(std::back_inserter(bb), "compress/decompress big data {}, ",
                   bb.size());
  }
  auto src = bytes(bb.begin(), bb.end());
  bytes cd;
  EncodingUtil::CompressZSTDLevel(cd, src, 7);
  bytes plain_data;
  EncodingUtil::DecompressZSTD(plain_data, cd);
  EXPECT_EQ(plain_data, src);
}

} // namespace mergekv
