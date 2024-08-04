#pragma once

#include "io.h"
#include "types.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <tuple>
#include <zstd.h>

namespace mergekv {

const auto kDstreamInBufferSize = ZSTD_DStreamInSize();
const auto kDstreamOutBufferSize = ZSTD_DStreamOutSize();

class EncodingUtil {
public:
  static void MarshalVarUint64(bytes &dst, uint64_t u);
  static void MarshalUint64(bytes &dst, uint64_t u);
  static uint64_t UnmarshalUint64(bytes_const_span src);
  static void MarshalVarUint64s(bytes &dst, u64s &ls);
  static void MarshalVarUint64sSlow(bytes &dst, u64s &ls);
  static mu64_bytes UnmarshalVarUint64(bytes_const_span src);
  static bytes_const_span UnmarshalVarUint64s(u64s &dst, bytes_const_span src);
  static bytes_const_span UnmarshalVarUint64sSlow(u64s &dst,
                                                  bytes_const_span src);
  static mu64_bytes Uvarint(bytes_const_span src);
  static void MarshalUint32(bytes &dst, uint32_t u);
  static uint32_t UnmarshalUint32(bytes_const_span src);

  static void MarshalBytes(bytes &dst, bytes_const_span src);
  static std::tuple<bytes_const_span, int> UnmarshalBytes(bytes_const_span src);

  static void CompressZSTDLevel(bytes &dst, bytes_const_span src, int level);
  static void DecompressZSTD(bytes &dst, bytes_const_span src);
  static void streamDecompressZSTD(bytes &dst, bytes_const_span src);
};

struct InBuffWrapper {
public:
  // forbid copy
  InBuffWrapper(const InBuffWrapper &) = delete;
  InBuffWrapper &operator=(const InBuffWrapper &) = delete;

  InBuffWrapper(size_t capacity) {
    in_buf_.src = new uint8_t[capacity];
    in_buf_.size = 0;
    in_buf_.pos = 0;
  }
  ~InBuffWrapper() {
    delete[] static_cast<uint8_t *>(const_cast<void *>(in_buf_.src));
  }

  size_t size() { return in_buf_.size; }
  void set_size(size_t size) { in_buf_.size = size; }
  size_t pos() { return in_buf_.pos; }
  void set_pos(size_t pos) { in_buf_.pos = pos; }

  ZSTD_inBuffer in_buf_;
};

struct OutBufWrapper {
public:
  // forbid copy
  OutBufWrapper(const OutBufWrapper &) = delete;
  OutBufWrapper &operator=(const OutBufWrapper &) = delete;

  OutBufWrapper(size_t capacity) {
    out_buf_.dst = new uint8_t[capacity];
    out_buf_.size = 0;
    out_buf_.pos = 0;
  }
  ~OutBufWrapper() { delete[] static_cast<uint8_t *>(out_buf_.dst); }

  size_t size() { return out_buf_.size; }
  void set_size(size_t size) { out_buf_.size = size; }
  size_t pos() { return out_buf_.pos; }
  void set_pos(size_t pos) { out_buf_.pos = pos; }

  ZSTD_outBuffer out_buf_;
};

class ZstdReader {
public:
  ZstdReader(bytes_const_span src);

  // copy is forbidden
  ZstdReader(const ZstdReader &) = delete;
  ZstdReader &operator=(const ZstdReader &) = delete;

  std::tuple<size_t, bool> Read(bytes &p);
  size_t WriteTo(bytes &p);

private:
  // if false, has more data to read, true, no more data to read(eof)
  bool FillOutBuf();
  bool FillInBuf();

private:
  std::unique_ptr<BytesReader> reader_;
  std::unique_ptr<ZSTD_DStream, std::function<void(ZSTD_DStream *)>> d_stream_;
  std::unique_ptr<InBuffWrapper> in_buf_;
  std::unique_ptr<OutBufWrapper> out_buf_;
};

} // namespace mergekv