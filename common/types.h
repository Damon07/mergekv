#pragma once

#include <span>
#include <vector>

namespace mergekv {
const int MaxVarintLen64 = 10;

using bytes = std::vector<uint8_t>;
using bytes_const_span = std::span<const uint8_t>;
using bytes_span = std::span<uint8_t>;
using u64s_span = std::span<uint64_t>;
using u64s = std::vector<uint64_t>;
using mu64_bytes = std::tuple<uint64_t, int>;
} // namespace mergekv