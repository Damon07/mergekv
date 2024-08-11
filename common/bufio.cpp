#include "bufio.h"
#include "exception.h"
#include "types.h"
#include <cstddef>
#include <cstring>

namespace mergekv {

void BufferWriter::Flush() {
  if (!err_msg_.empty()) {
    throw IOException(err_msg_);
  }
  if (size_ == 0) {
    return;
  }

  size_t n = 0;
  try {
    n = w_->Write(bytes_const_span(buf_.data(), size_));
    if (n != size_) {
      err_msg_ = "short write";
    }
  } catch (const std::exception &e) {
    err_msg_ = e.what();
    throw;
  }

  if (!err_msg_.empty()) {
    if (n > 0 && n < size_) {
      std::memmove(buf_.data(), buf_.data() + n, size_ - n);
    }
    size_ -= n;
    throw IOException(err_msg_);
  }
  size_ = 0;
}

size_t BufferWriter::Write(bytes_const_span p) {
  size_t nn = 0;
  while (p.size() > available() && err_msg_.empty()) {
    size_t n = 0;
    if (buffered() == 0) {
      // Large write, empty buffer.
      // Write directly from p to avoid copy.
      try {
        n = w_->Write(p);
      } catch (const IOException &e) {
        err_msg_ = e.what();
      }
    } else {
      auto cp_size = available();
      std::memcpy(buf_.data() + size_, p.data(), cp_size);
      size_ += cp_size;
      Flush();
    }
    nn += n;
    p = p.subspan(n);
  }
  if (!err_msg_.empty()) {
    throw IOException(err_msg_);
  }

  auto cp_size = available();
  std::memcpy(buf_.data() + size_, p.data(), cp_size);
  size_ += cp_size;
  return nn + cp_size;
}

} // namespace mergekv