#pragma once

#include "fmt/core.h"
#include "io.h"
#include "types.h"
#include <cstddef>
#include <memory>

namespace mergekv {
class ByteBuffer;

class ByteBufferReader : public FileReader {
public:
  ByteBufferReader(const ByteBufferReader &) = delete;
  ByteBufferReader &operator=(const ByteBufferReader &) = delete;
  ByteBufferReader(std::shared_ptr<bytes> data, const string &path)
      : data_(data), read_pos_(0), path_(path){};

  string Path() const override;
  size_t Read(bytes &p) override;

private:
  std::shared_ptr<bytes> data_;
  size_t read_pos_ = 0;
  string path_;
};

class ByteBuffer : public FileReader {
public:
  // forbit copy
  ByteBuffer(const ByteBuffer &) = delete;
  ByteBuffer &operator=(const ByteBuffer &) = delete;
  ByteBuffer() = default;

  string Path() const override {
    return fmt::format("ByteBuffer/{:x}/mem",
                       reinterpret_cast<std::uintptr_t>(this));
  }

  void Reset() { data_->clear(); }

  size_t Write(bytes_const_span p);

  void MustReadAt(bytes &p, size_t off);

  size_t ReadFrom(Reader &r);

  size_t Read(bytes &p) override;

  std::unique_ptr<FileReader> NewReader() {
    return std::make_unique<ByteBufferReader>(data_, Path());
  }

  size_t size() const { return data_->size(); }

  bytes_const_span data() const { return *data_; }

private:
  std::shared_ptr<bytes> data_;
};

} // namespace mergekv