#include "io.h"
#include <cstddef>
#include <cstring>
#include <tuple>

namespace mergekv {

// ReadAll reads from r until eof and returns the data it read.
void Reader::ReadAll(bytes &dst, Reader &r) {
  if (dst.size() == 0) {
    dst.reserve(512);
  }
  while (true) {
    auto start = dst.size();
    auto [n, eof] = r.Read(dst.data() + start, dst.capacity() - start);
    dst.resize(start + n);
    if (eof) {
      break;
    }
    if (dst.size() == dst.capacity()) {
      auto pre_size = dst.size();
      dst.push_back(0);
      dst.resize(pre_size);
    }
  }
}

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

// read n bytes from src_ to buf, if eof, return false
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