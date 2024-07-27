#include "io.h"
#include <cstddef>
#include <tuple>

namespace mergekv {

// read n bytes from src_ to data, if eof, return true
std::tuple<size_t, bool> BytesReader::Read(void *data, size_t n) {
  if (src_.empty()) {
    return {0, true};
  }

  if (n > src_.size()) {
    n = src_.size();
  }

  std::memcpy(data, src_.data(), n);
  src_ = src_.subspan(n);
  return {n, false};
}

// read n bytes from src_ to buf, if eof, return -1
std::tuple<size_t, bool> BytesReader::Read(bytes &buf) {
  if (src_.empty()) {
    return {0, true};
  }

  auto n = buf.size();
  if (n > src_.size()) {
    n = src_.size();
  }

  buf = bytes(src_.begin(), src_.begin() + n);
  src_ = src_.subspan(n);
  return {n, false};
}
} // namespace mergekv