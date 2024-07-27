#include "gtest/gtest.h"

#include "add.h"
#include "encoding_util.h"
#include "string_util.h"

// int add(int a, int b) { return a + b; }
namespace mergekv {
TEST(AddTest, PositiveNumbers) {
  EXPECT_EQ(add(1, 1), 2);
  EXPECT_EQ(add(2, 2), 4);
  EXPECT_EQ(add(3, 3), 6);
}

void testMarshalUnmarshalVarUint64(uint64_t u) {
  bytes dst;
  EncodingUtil::MarshalVarUint64(dst, u);
  auto [u_new, n] = EncodingUtil::UnmarshalVarUint64(dst);
  if (n <= 0) {
    FAIL() << "unexpected error when unmarshaling u=" << u
           << " from dst=" << StringUtil::Format(dst);
  }
  EXPECT_EQ(u, u_new);
}

TEST(MarshalUnmarshal, VarUint64) {
  testMarshalUnmarshalVarUint64(0);
  testMarshalUnmarshalVarUint64(1);
  testMarshalUnmarshalVarUint64((1 << 6) - 1);
  testMarshalUnmarshalVarUint64(1 << 6);
  testMarshalUnmarshalVarUint64((uint64_t(1) << 63) - 1);

  for (uint64_t i = 0; i < 1024; i++) {
    testMarshalUnmarshalVarUint64(i);
    testMarshalUnmarshalVarUint64(i << 8);
    testMarshalUnmarshalVarUint64(i << 16);
    testMarshalUnmarshalVarUint64(i << 23);
    testMarshalUnmarshalVarUint64(i << 33);
    testMarshalUnmarshalVarUint64(i << 35);
    testMarshalUnmarshalVarUint64(i << 41);
    testMarshalUnmarshalVarUint64(i << 49);
    testMarshalUnmarshalVarUint64(i << 54);
  }
}
} // namespace mergekv
