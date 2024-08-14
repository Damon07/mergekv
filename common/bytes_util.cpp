#include "bytes_util.h"
#include "exception.h"
#include "types.h"
#include <cstddef>
#include <cstring>

namespace mergekv {

string ByteBufferReader::Path() const { return path_; }

size_t ByteBufferReader::Read(bytes &p) {
  size_t n = std::min(p.size(), data_->size() - read_pos_);
  std::copy(data_->begin() + read_pos_, data_->begin() + read_pos_ + n,
            p.begin());
  read_pos_ += n;
  return n;
}

size_t ByteBuffer::Write(bytes_const_span p) {
  data_->insert(data_->end(), p.begin(), p.end());
  return p.size();
}

void ByteBuffer::MustReadAt(bytes &p, size_t offset) {
  if (data_->size() < offset + p.size()) {
    throw InvalidInputException(
        "BUG: too big offset=%d; can not exceed size=%d", offset,
        data_->size());
  }

  std::memcpy(p.data(), data_->data(), p.size());
}

size_t ByteBuffer::ReadFrom(Reader &r) {
  auto len = data_->size();
  data_->resize(4 * 1024);
  auto offset = len;
  while (true) {
    auto free = data_->size() - offset;
    if (free < offset) {
      data_->resize(data_->size() * 2);
    }
    data_->resize(offset);
    auto [n, eof] = r.Read(*data_);
    offset += n;
    if (eof) {
      data_->resize(offset);
      return offset - len;
    }
  }
}

} // namespace mergekv