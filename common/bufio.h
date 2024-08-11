#pragma once

#include "io.h"
#include "types.h"
#include <cstddef>
#include <memory>

namespace mergekv {
class BufferWriter {
public:
  // forbit copy
  BufferWriter(const BufferWriter &) = delete;
  BufferWriter &operator=(const BufferWriter &) = delete;

  BufferWriter(std::shared_ptr<Writer> w, size_t size)
      : w_(w), size_(size), buf_(size) {}

  size_t size() const { return size_; }
  void Flush();
  size_t available() const { return buf_.size() - size_; }
  size_t buffered() const { return size_; }
  size_t Write(bytes_const_span p);

private:
  string err_msg_;
  bytes buf_;
  size_t size_;
  std::shared_ptr<Writer> w_;
};

} // namespace mergekv