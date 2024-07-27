#include "encoding_util.h"
#include "exception.h"
#include "types.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <tuple>
#include <zstd.h>
#include <zstd_errors.h>

namespace mergekv {
void EncodingUtil::MarshalVarUint64(bytes &dst, uint64_t u) {
  if (u < (1 << 7)) {
    dst.push_back(uint8_t(u));
    return;
  }
  if (u < (1 << 2 * 7)) {
    dst.insert(dst.end(), {uint8_t(u | 0x80), uint8_t(u >> 7)});
    return;
  }
  if (u < (1 << 3 * 7)) {
    dst.insert(dst.end(), {uint8_t(u | 0x80), uint8_t((u >> 7) | 0x80),
                           uint8_t(u >> 2 * 7)});
    return;
  }
  // Slow path for big intergers
  auto tmp = u64s(1, u);
  MarshalVarUint64s(dst, tmp);
}

void EncodingUtil::MarshalVarUint64s(bytes &dst, u64s &ls) {
  auto start = dst.size();
  for (auto l : ls) {
    if (l >= (1 << 7)) {
      dst.resize(start);
      return MarshalVarUint64sSlow(dst, ls);
    }
    dst.push_back(uint8_t(l));
  }
}

void EncodingUtil::MarshalVarUint64sSlow(bytes &dst, u64s &ls) {
  for (auto l : ls) {
    if (l < (1 << 7)) {
      dst.push_back(uint8_t(l));
      continue;
    }
    if (l < (1 << 2 * 7)) {
      dst.insert(dst.end(), {uint8_t(l | 0x80), uint8_t(l >> 7)});
      continue;
    }
    if (l < (1 << (3 * 7))) {
      dst.insert(dst.end(), {uint8_t(l | 0x80), uint8_t((l >> 7) | 0x80),
                             uint8_t(l >> 2 * 7)});
      continue;
    }
    if (l >= (uint64_t(1) << (8 * 7))) {
      if (l < (uint64_t(1) << 9 * 7)) {
        dst.insert(dst.end(),
                   {uint8_t(l | 0x80), uint8_t((l >> 7) | 0x80),
                    uint8_t((l >> 2 * 7) | 0x80), uint8_t((l >> 3 * 7) | 0x80),
                    uint8_t((l >> 4 * 7) | 0x80), uint8_t((l >> 5 * 7) | 0x80),
                    uint8_t((l >> 6 * 7) | 0x80), uint8_t((l >> 7 * 7) | 0x80),
                    uint8_t((l >> 8 * 7))});
        continue;
      }

      dst.insert(dst.end(),
                 {uint8_t(l | 0x80), uint8_t((l >> 7) | 0x80),
                  uint8_t((l >> 2 * 7) | 0x80), uint8_t((l >> 3 * 7) | 0x80),
                  uint8_t((l >> 4 * 7) | 0x80), uint8_t((l >> 5 * 7) | 0x80),
                  uint8_t((l >> 6 * 7) | 0x80), uint8_t((l >> 7 * 7) | 0x80),
                  uint8_t((l >> 8 * 7) | 0x80), 1});

      continue;
    }
    if (l < (1 << 4 * 7)) {
      dst.insert(dst.end(),
                 {uint8_t(l | 0x80), uint8_t((l >> 7) | 0x80),
                  uint8_t((l >> 2 * 7) | 0x80), uint8_t((l >> 3 * 7))});
      continue;
    }

    if (l < (uint64_t(1) << 5 * 7)) {
      dst.insert(dst.end(),
                 {uint8_t(l | 0x80), uint8_t((l >> 7) | 0x80),
                  uint8_t((l >> 2 * 7) | 0x80), uint8_t((l >> 3 * 7) | 0x80),
                  uint8_t((l >> 4 * 7))});
      continue;
    }

    if (l < (uint64_t(1) << 6 * 7)) {
      dst.insert(dst.end(),
                 {uint8_t(l | 0x80), uint8_t((l >> 7) | 0x80),
                  uint8_t((l >> 2 * 7) | 0x80), uint8_t((l >> 3 * 7) | 0x80),
                  uint8_t((l >> 4 * 7) | 0x80), uint8_t((l >> 5 * 7))});
      continue;
    }

    if (l < (uint64_t(1) << 7 * 7)) {
      dst.insert(dst.end(),
                 {uint8_t(l | 0x80), uint8_t((l >> 7) | 0x80),
                  uint8_t((l >> 2 * 7) | 0x80), uint8_t((l >> 3 * 7) | 0x80),
                  uint8_t((l >> 4 * 7) | 0x80), uint8_t((l >> 5 * 7) | 0x80),
                  uint8_t((l >> 6 * 7))});
      continue;
    }

    dst.insert(dst.end(),
               {uint8_t(l | 0x80), uint8_t((l >> 7) | 0x80),
                uint8_t((l >> 2 * 7) | 0x80), uint8_t((l >> 3 * 7) | 0x80),
                uint8_t((l >> 4 * 7) | 0x80), uint8_t((l >> 5 * 7) | 0x80),
                uint8_t((l >> 6 * 7) | 0x80), uint8_t((l >> 7 * 7))});
  }
}

void EncodingUtil::MarshalUint64(bytes &dst, uint64_t u) {
  dst.insert(dst.end(), {uint8_t(u >> 56), uint8_t(u >> 48), uint8_t(u >> 40),
                         uint8_t(u >> 32), uint8_t(u >> 24), uint8_t(u >> 16),
                         uint8_t(u >> 8), uint8_t(u)});
}

uint64_t EncodingUtil::UnmarshalUint64(bytes_const_span src) {
  return static_cast<uint64_t>(src[0]) << 56 |
         static_cast<uint64_t>(src[1]) << 48 |
         static_cast<uint64_t>(src[2]) << 40 |
         static_cast<uint64_t>(src[3]) << 32 |
         static_cast<uint64_t>(src[4]) << 24 |
         static_cast<uint64_t>(src[5]) << 16 |
         static_cast<uint64_t>(src[6]) << 8 | static_cast<uint64_t>(src[7]);
}

mu64_bytes EncodingUtil::UnmarshalVarUint64(bytes_const_span src) {
  if (src.empty()) {
    return {0, 0};
  }

  if (src[0] < 0x80) {
    return {src[0], 1};
  }
  if (src.size() == 1) {
    return {0, 0};
  }
  if (src[1] < 0x80) {
    return {uint64_t(src[0] & 0x7f) | uint64_t(src[1]) << 7, 2};
  }
  // slow path for other number of bytes
  return Uvarint(src);
}

mu64_bytes EncodingUtil::Uvarint(bytes_const_span src) {
  uint64_t x = 0;
  size_t s = 0;
  for (size_t i = 0; i < src.size(); i++) {
    uint8_t b = src[i];
    if (i == MaxVarintLen64) {
      // Catch byte reads past MaxVarintLen64.
      // See issue https://golang.org/issues/41185
      return {0, -(i + 1)}; // overflow
    }

    if (b < 0x80) {
      if (i == MaxVarintLen64 - 1 && b > 1) {
        return {0, -(i + 1)}; // overflow
      }
      return {x | uint64_t(b) << s, i + 1};
    }
    x |= uint64_t(b & 0x7f) << s;
    s += 7;
  }
  return {0, 0};
}

bytes_const_span EncodingUtil::UnmarshalVarUint64s(u64s &dst,
                                                   bytes_const_span src) {
  if (src.size() < dst.size()) {
    throw InvalidInputException(
        "UnmarshalVarUint64s: src size is smaller than ls size");
  }

  for (size_t i = 0; i < dst.size(); i++) {
    auto c = src[i];
    if (c >= 0x80) {
      return UnmarshalVarUint64sSlow(dst, src);
    }
    dst[i] = c;
  }
  return src.subspan(dst.size());
}

bytes_const_span EncodingUtil::UnmarshalVarUint64sSlow(u64s &dst,
                                                       bytes_const_span src) {
  auto idx = size_t(0);
  for (size_t i = 0; i < dst.size(); i++) {
    if (idx >= src.size()) {
      throw InvalidInputException("cannot unmarshal varuint from empty data");
    }
    auto c = src[idx];
    idx++;
    if (c < 0x80) {
      dst[i] = c;
      continue;
    }

    if (idx >= src.size()) {
      throw InvalidInputException(
          "unexpected end of encoded varuint at byte 1; src=%x",
          src.subspan(idx - 1));
    }
    auto d = src[idx];
    idx++;
    if (d < 0x80) {
      dst[i] = uint64_t(c & 0x7f) | uint64_t(d) << 7;
      continue;
    }

    if (idx >= src.size()) {
      throw InvalidInputException(
          "unexpected end of encoded varuint at byte 2; src=%x",
          src.subspan(idx - 2));
    }
    auto e = src[idx];
    idx++;
    if (e < 0x80) {
      dst[i] =
          uint64_t(c & 0x7f) | uint64_t(d & 0x7f) << 7 | uint64_t(e) << 2 * 14;
      continue;
    }

    auto u = uint64_t(c & 0x7f) | uint64_t(d & 0x7f) << 7 |
             uint64_t(e & 0x7f) << 2 * 7;

    // Slow path
    auto j = idx;
    while (1) {
      if (idx >= src.size()) {
        throw InvalidInputException(
            "unexpected end of encoded varuint at byte %d; src=%x", idx,
            src.subspan(j - 3));
      }
      auto c = src[idx];
      idx++;
      if (c < 0x80) {
        break;
      }
    }
    // These are the most common cases
    switch (idx - j) {
    case 1: {
      u |= uint64_t(src[j]) << 3 * 7;
    } break;
    case 2: {
      auto b = src.subspan(j, 2);
      u |= uint64_t(b[0] & 0x7f) << 3 * 7 | uint64_t(b[1]) << 4 * 7;
    } break;
    case 3: {
      auto b = src.subspan(j, 3);
      u |= uint64_t(b[0] & 0x7f) << 3 * 7 | uint64_t(b[1] & 0x7f) << 4 * 7 |
           uint64_t(b[2]) << 5 * 7;
    } break;
    case 4: {
      auto b = src.subspan(j, 4);
      u |= uint64_t(b[0] & 0x7f) << 3 * 7 | uint64_t(b[1] & 0x7f) << 4 * 7 |
           uint64_t(b[2] & 0x7f) << 5 * 7 | uint64_t(b[3]) << 6 * 7;
    } break;
    case 5: {
      auto b = src.subspan(j, 5);
      u |= uint64_t(b[0] & 0x7f) << 3 * 7 | uint64_t(b[1] & 0x7f) << 4 * 7 |
           uint64_t(b[2] & 0x7f) << 5 * 7 | uint64_t(b[3] & 0x7f) << 6 * 7 |
           uint64_t(b[4]) << 7 * 7;
    } break;
    case 6: {
      auto b = src.subspan(j, 6);
      u |= uint64_t(b[0] & 0x7f) << 3 * 7 | uint64_t(b[1] & 0x7f) << 4 * 7 |
           uint64_t(b[2] & 0x7f) << 5 * 7 | uint64_t(b[3] & 0x7f) << 6 * 7 |
           uint64_t(b[4] & 0x7f) << 7 * 7 | uint64_t(b[5]) << 8 * 7;
    } break;
    case 7: {
      auto b = src.subspan(j, 7);
      if (b[6] > 1) {
        throw InvalidInputException("too big encoded varuint; src=%x",
                                    src.subspan(j - 3));
      }
      u |= uint64_t(b[0] & 0x7f) << 3 * 7 | uint64_t(b[1] & 0x7f) << 4 * 7 |
           uint64_t(b[2] & 0x7f) << 5 * 7 | uint64_t(b[3] & 0x7f) << 6 * 7 |
           uint64_t(b[4] & 0x7f) << 7 * 7 | uint64_t(b[5] & 0x7f) << 8 * 7 |
           uint64_t(1) << 9 * 7;
    } break;
    default:
      throw InvalidInputException(
          "too long encoded varuint; the maximum allowed length is 10 bytes; "
          "got %d bytes; src=%x",
          idx - j + 3, src.subspan(j - 3));
    }
    dst[i] = u;
  }
  return src.subspan(idx);
}

void EncodingUtil::CompressZSTDLevel(bytes &dst, bytes_const_span src,
                                     int level) {
  if (src.empty()) {
    return;
  }
  auto dst_len = dst.size();
  if (dst.capacity() > dst_len) {
    // Fast path
    // make sure dst has enough size
    dst.resize(dst.capacity());
    auto result = ZSTD_compress(dst.data() + dst_len, dst.size() - dst_len,
                                src.data(), src.size(), level);
    auto compress_size = result;
    if (compress_size >= 0) {
      dst.resize(dst_len + compress_size);
      return;
    }

    if (ZSTD_getErrorCode(result) != ZSTD_error_dstSize_tooSmall) {
      throw FatalException("ZSTD_compress: %s", ZSTD_getErrorName(result));
    }
  }

  // Slow path, reallocate dst
  auto compress_bound = ZSTD_compressBound(src.size()) + 1;
  auto n = dst_len + compress_bound - dst.capacity() + dst_len;
  if (n > 0) {
    dst.resize(dst.size() + n);
  }
  auto result = ZSTD_compress(dst.data() + dst_len, compress_bound, src.data(),
                              src.size(), level);
  auto compress_size = result;
  dst.resize(dst_len + compress_size);
}

void EncodingUtil::DecompressZSTD(bytes &dst, bytes_const_span src) {
  if (src.empty()) {
    return;
  }
  auto dst_len = dst.size();
  if (dst.capacity() > dst_len) {
    // Fast path
    // make sure dst has enough size
    // dst.resize(dst.capacity());
    auto result = ZSTD_decompress(dst.data() + dst_len, dst.size() - dst_len,
                                  src.data(), src.size());
    auto decompress_size = int(result);
    if (decompress_size >= 0) {
      dst.resize(dst_len + decompress_size);
      return;
    }

    if (ZSTD_getErrorCode(result) != ZSTD_error_dstSize_tooSmall) {
      throw InternalException("ZSTD_decompress: %s", ZSTD_getErrorName(result));
    }
  }

  // Slow path, reallocate dst
  auto r = ZSTD_getFrameContentSize(src.data(), src.size());

  auto decompress_bound = int(r);
  switch (r) {
  case ZSTD_CONTENTSIZE_ERROR:
    throw InvalidInputException("cannot decompress invalid src");
  case ZSTD_CONTENTSIZE_UNKNOWN:
    streamDecompressZSTD(dst, src);
    return;
  }

  decompress_bound++;

  auto n = int(dst_len + decompress_bound - dst.capacity());
  if (n > 0) {
    dst.resize(dst.size() + n);
  }
  dst.resize(dst_len + decompress_bound);
  auto result = ZSTD_decompress(dst.data() + dst_len, decompress_bound,
                                src.data(), src.size());
  auto decompress_size = int(result);
  if (decompress_size >= 0) {
    dst.resize(dst_len + decompress_size);
    return;
  }

  throw InvalidInputException("ZSTD_decompress fail: %s",
                              ZSTD_getErrorName(result));
}

void EncodingUtil::streamDecompressZSTD(bytes &dst, bytes_const_span src) {
  ZstdReader reader(src);
  reader.WriteTo(dst);
}

ZstdReader::ZstdReader(bytes_const_span src) {
  reader_ = std::make_unique<BytesReader>(src);
  auto deleter = [](ZSTD_DStream *dStream) { ZSTD_freeDStream(dStream); };
  d_stream_ = std::unique_ptr<ZSTD_DStream, decltype(deleter)>(
      ZSTD_createDStream(), deleter);

  in_buf_ = std::make_unique<InBuffWrapper>(kDstreamInBufferSize);
  out_buf_ = std::make_unique<OutBufWrapper>(kDstreamOutBufferSize);
}

bool ZstdReader::FillOutBuf() {
  if (in_buf_->pos() == in_buf_->size() &&
      out_buf_->size() < kDstreamOutBufferSize) {
    if (FillInBuf()) {
      return true;
    }
  }

  while (true) {
    out_buf_->set_size(kDstreamOutBufferSize);
    out_buf_->set_pos(0);
    auto pre_in_buf_pos = in_buf_->pos();
    auto result = ZSTD_decompressStream(d_stream_.get(), &(out_buf_->out_buf_),
                                        &(in_buf_->in_buf_));
    out_buf_->set_size(out_buf_->pos());
    out_buf_->set_pos(0);
    if (ZSTD_getErrorCode(result) != 0) {
      throw InvalidInputException("ZSTD_decompressStream: %s",
                                  ZSTD_getErrorName(result));
    }

    if (out_buf_->size() > 0) {
      return false;
    }

    // nothing in out_buf_, data is consumed from in_buf_, but decompressed into
    // nothing. And there is more data in in_buf_, continue to decompress
    if (in_buf_->pos() != pre_in_buf_pos /*has consumed*/ &&
        in_buf_->pos() < in_buf_->size() /*has more data*/) {
      continue;
    }

    auto eof = FillInBuf();
    if (eof) {
      return true;
    }
  }
}
bool ZstdReader::FillInBuf() {
  auto base = const_cast<void *>(in_buf_->in_buf_.src);
  std::memmove(base, static_cast<uint8_t *>(base) + in_buf_->pos(),
               in_buf_->size() - in_buf_->pos());
  in_buf_->set_size(in_buf_->size() - in_buf_->pos());
  in_buf_->set_pos(0);
  while (true) {
    auto [n, eof] = reader_->Read(
        static_cast<uint8_t *>(base) + in_buf_->size(), kDstreamInBufferSize);
    in_buf_->set_size(in_buf_->size() + n);
    if (!eof) {
      if (n == 0) {
        continue;
      }
    }

    if (n > 0) {
      return false;
    }

    if (eof) {
      return true;
    }

    throw InvalidInputException("FillInBuf: can not read data from reader");
  }
}

std::tuple<size_t, bool> ZstdReader::Read(bytes &p) {
  if (p.empty()) {
    return {0, false};
  }

  if (out_buf_->size() == 0) {
    if (FillOutBuf()) {
      return {0, true};
    }
  }

  std::memcpy(p.data(), out_buf_->out_buf_.dst,
              out_buf_->size() - out_buf_->pos());
  out_buf_->set_pos(out_buf_->size());
  return {out_buf_->size() - out_buf_->pos(), false};
}

size_t ZstdReader::WriteTo(bytes &p) {
  size_t n = 0;
  while (true) {
    if (out_buf_->pos() == out_buf_->size()) {
      if (FillOutBuf()) {
        return n;
      }
    }

    p.reserve(p.size() + out_buf_->size() - out_buf_->pos());
    auto start =
        static_cast<uint8_t *>(out_buf_->out_buf_.dst) + out_buf_->pos();
    auto end = start + (out_buf_->size() - out_buf_->pos());

    p.insert(p.end(), start, end);

    n += out_buf_->size() - out_buf_->pos();
    out_buf_->set_pos(out_buf_->size());
  }
}

} // namespace mergekv