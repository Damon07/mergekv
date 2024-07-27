#include "gtest/gtest.h"
#include <fmt/core.h>

int main(int argc, char **argv) {
  fmt::print("Hello World!\n");

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}