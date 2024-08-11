#include "file.h"
#include "exception.h"
#include "io.h"
#include "memory.h"
#include <cstddef>
#include <exception>
#include <filesystem>
#include <memory>
#include <mutex>
#include <sys/_types/_mode_t.h>
#include <sys/fcntl.h>
#include <unistd.h>

namespace mergekv {

std::once_flag BufferFileWriter::once_flag;

size_t BufferFileWriter::GetBufferSize() {
  static size_t buffer_size = 0;
  std::call_once(once_flag, []() {
    auto n = CgroupUtil::AllowedMemory() / 1024 / 8;
    if (n < 4 * 1024) {
      n = 4 * 1024;
    }
    if (n > 512 * 1024) {
      n = 512 * 1024;
    }
    buffer_size = n;
  });
  return buffer_size;
}

BufferFileWriter::BufferFileWriter(const string &filename, int flags,
                                   mode_t mode)
    : filename_(filename), fd_(-1) {
#ifdef OS_DARWIN
  bool o_direct = (flags != -1) && (flags & O_DIRECT);
  if (o_direct)
    flags = flags & ~O_DIRECT;
#endif
  fd_ = ::open(filename.c_str(),
               flags == -1 ? O_WRONLY | O_TRUNC | O_CREAT | O_CLOEXEC
                           : flags | O_CLOEXEC,
               mode);
  if (-1 == fd_) {
    throw IOException("can not open file: %s, errno: %d", filename.c_str(),
                      errno);
  }

#ifdef OS_DARWIN
  if (o_direct) {
    if (fcntl(fd_, F_NOCACHE, 1) == -1) {
      throw IOException("can not set F_NOCACHE for file: %s, errno: %d",
                        filename.c_str(), errno);
    }
  }
#endif

  bw_ = std::make_unique<BufferWriter>(std::make_shared<FileDescWriter>(fd_),
                                       GetBufferSize());
}

size_t BufferFileWriter::Write(bytes_const_span p) { return bw_->Write(p); }

void BufferFileWriter::MustFlush(bool sync) {
  try {
    bw_->Flush();
    if (sync) {
      if (fsync(fd_) == -1) {
        throw IOException("can not sync file: %s, errno: %d", filename_.c_str(),
                          errno);
      }
    }
  } catch (const std::exception &e) {
    throw FatalException("can not flush file: %s, %s", filename_.c_str(),
                         e.what());
  }
}

void BufferFileWriter::MustClose() {
  try {
    bw_->Flush();
    if (fsync(fd_) == -1) {
      throw IOException("can not sync file: %s, errno: %d", filename_.c_str(),
                        errno);
    }
    if (close(fd_) == -1) {
      throw IOException("can not close file: %s, errno: %d", filename_.c_str(),
                        errno);
    }
    fd_ = -1;
  } catch (const std::exception &e) {
    fd_ = -1;
    throw FatalException("can not close file: %s, %s", filename_.c_str(),
                         e.what());
  }
}

void BufferFileWriter::MustSync(bool sync) {
  if (sync) {
    if (fsync(fd_) == -1) {
      throw IOException("can not sync file: %s, errno: %d", filename_.c_str(),
                        errno);
    }
  }
}

BufferFileWriter::~BufferFileWriter() {
  if (fd_ != -1) {
    MustClose();
  }
}

std::atomic<uint64_t> FileUtils::tmp_file_num(0);

void FileUtils::MustSyncPath(const string &path) {
  auto fd = ::open(path.c_str(), O_RDONLY, 0);
  if (-1 == fd) {
    throw FatalException("can not open file: %s, errno: %d", path.c_str(),
                         errno);
  }
  if (fsync(fd) == -1) {
    close(fd);
    throw FatalException("can not sync file: %s, errno: %d", path.c_str(),
                         errno);
  }
  if (close(fd) == -1) {
    throw FatalException("can not close file: %s, errno: %d", path.c_str(),
                         errno);
  }
}

void FileUtils::MustWriteSync(const string &filename, bytes_const_span p) {
  try {
    BufferFileWriter bw(filename);
    bw.Write(p);
  } catch (const std::exception &e) {
    throw FatalException("can not write file: %s, %s", filename.c_str(),
                         e.what());
  }
}

void FileUtils::MustWriteAtomic(const string &filename, bytes_const_span p,
                                bool overwrite) {
  if (IsPathExist(filename) && !overwrite) {
    throw FatalException("file already exists: %s", filename.c_str());
  }
  tmp_file_num++;
  auto n = tmp_file_num.load();
  string tmp_filename = filename + ".tmp." + std::to_string(n);
  MustWriteSync(tmp_filename, p);
  try {
    fs::rename(tmp_filename, filename);
  } catch (const std::exception &e) {
    throw FatalException("can not rename file: %s, %s", filename.c_str(),
                         e.what());
  }

  fs::path abs_path;
  try {
    abs_path = fs::absolute(filename);
  } catch (const std::exception &e) {
    throw FatalException("can not get absolute path: %s, %s", filename.c_str(),
                         e.what());
  }
  auto parent_path = abs_path.parent_path();
  MustSyncPath(parent_path.string());
}
} // namespace mergekv