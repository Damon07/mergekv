#pragma once

#include "bufio.h"
#include "exception.h"
#include "io.h"
#include "types.h"
#include <cerrno>
#include <cstddef>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <unistd.h>

namespace mergekv {

class FileDescWriter : public Writer {
public:
  FileDescWriter(int fd) : fd_(fd) {}

  size_t Write(bytes_const_span p) override {
    auto res = ::write(fd_, p.data(), p.size());
    if ((-1 == res || 0 == res) && errno != EINTR) {
      throw IOException("can not write to file: %s, fd: %d", &filename_, fd_);
    }

    if (res > 0) {
      return res;
    }

    return 0;
  }

private:
  int fd_;
  string filename_;
};

class BufferFileWriter : public FileWriter {
public:
  // forbit copy
  BufferFileWriter(const BufferFileWriter &) = delete;
  BufferFileWriter &operator=(const BufferFileWriter &) = delete;

  BufferFileWriter(const string &filename, int flag = -1, mode_t mode = 0666);

  size_t Write(bytes_const_span p) override;
  void MustFlush(bool sync);
  void MustClose() override;
  string Path() const override { return filename_; }
  void MustSync(bool sync) override;
  ~BufferFileWriter() override;

private:
  static size_t GetBufferSize();
  static std::once_flag once_flag;
  int fd_;
  std::unique_ptr<BufferWriter> bw_;
  string filename_;
};

class FileUtils {
public:
  static void MustSyncPath(const string &path);
  static void MustWriteSync(const string &filename, bytes_const_span p);
  static void MustWriteAtomic(const string &filename, bytes_const_span p,
                              bool overwrite);
  static bool IsPathExist(const string &path);
  static size_t MustFileSize(const string &filename);

private:
  static std::atomic<uint64_t> tmp_file_num;
  static void MustMkdirSync(const string &path);
};

} // namespace mergekv