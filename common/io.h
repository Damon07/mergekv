#pragma once

#include "types.h"
#include <cstddef>
#include <tuple>

namespace mergekv {

#ifndef O_DIRECT
#define O_DIRECT 00040000
#endif

class Reader {
public:
  virtual ~Reader() = default;
  virtual std::tuple<size_t, bool> Read(void *data, size_t n) = 0;
  virtual std::tuple<size_t, bool> Read(bytes &buf) = 0;
  static void ReadAll(bytes &dst, Reader &r);
};

class BytesReader : public Reader {
public:
  BytesReader(bytes_const_span src) : src_(src) {}

  // copy is forbidden
  BytesReader(const BytesReader &) = delete;
  BytesReader &operator=(const BytesReader &) = delete;

  std::tuple<size_t, bool> Read(void *data, size_t n) override;
  std::tuple<size_t, bool> Read(bytes &buf) override;

private:
  bytes_const_span src_;
};

class FileReader {
public:
  virtual ~FileReader() = default;
  virtual string Path() const = 0;
  virtual size_t Read(bytes &p) = 0;
  virtual void MustClose(){};
};

class Writer {
public:
  virtual ~Writer() = default;
  virtual size_t Write(bytes_const_span p) = 0;
};

class FileWriter : public Writer {
public:
  virtual ~FileWriter() = default;
  virtual string Path() const = 0;
  virtual void MustClose() = 0;
  virtual void MustSync(bool sync) = 0;
};

} // namespace mergekv
