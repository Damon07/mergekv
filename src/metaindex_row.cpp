#include "metaindex_row.h"
#include "encoding_util.h"
#include "exception.h"
#include "io.h"
#include "string_util.h"
#include "types.h"
#include <algorithm>
#include <span>

namespace mergekv {

bytes_const_span MetaIndexRow::Unmarshal(bytes_const_span src) {
  auto [fi, n_size] = EncodingUtil::UnmarshalBytes(src);
  if (n_size <= 0) {
    throw InvalidInputException("cannot unmarshal firstItem");
  }

  src = src.subspan(n_size);
  first_item.insert(first_item.end(), fi.begin(), fi.end());

  if (src.size() < sizeof(uint32_t)) {
    throw InvalidInputException("cannot unmarshal bhsCount");
  }

  bhs_count = EncodingUtil::UnmarshalUint32(src);
  src = src.subspan(sizeof(uint32_t));

  if (src.size() < sizeof(uint64_t)) {
    throw InvalidInputException("cannot unmarshal indexBlockOffset");
  }

  index_block_offset = EncodingUtil::UnmarshalUint64(src);
  src = src.subspan(sizeof(uint64_t));

  if (src.size() < sizeof(uint32_t)) {
    throw InvalidInputException("cannot unmarshal indexBlockSize");
  }

  index_block_size = EncodingUtil::UnmarshalUint32(src);
  src = src.subspan(sizeof(uint32_t));

  if (bhs_count < 1) {
    throw InvalidInputException("bhsCount must be greater than 0");
  }

  if (index_block_size > 4 * kMaxIndexBlockSize) {
    throw InvalidInputException(
        "indexBlockSize is too large: %d; cannot be greater than %d",
        index_block_size, 4 * kMaxIndexBlockSize);
  }

  return src;
}

void MetaIndexRow::UnmarshalMIRows(std::vector<MetaIndexRow> &dst, Reader &r) {
  bytes data, de_data;
  Reader::ReadAll(data, r);
  EncodingUtil::DecompressZSTD(de_data, data);

  auto dst_len = dst.size();

  auto data_span = bytes_const_span(data);
  while (data_span.size() > 0) {
    if (dst.size() < dst.capacity()) {
      dst.resize(dst.size() + 1);
    } else {
      dst.emplace_back();
    }
    auto &mr = dst[dst.size() - 1];
    data_span = mr.Unmarshal(data_span);
  }

  if (dst_len == dst.size()) {
    throw InvalidInputException("expecting non-zero metaindex rows; got zero");
  }

  auto ok = std::is_sorted(dst.begin() + dst_len, dst.end(),
                           [](MetaIndexRow &a, MetaIndexRow &b) {
                             return StringUtil::ToStringView(a.first_item) <
                                    StringUtil::ToStringView(b.first_item);
                           });
  if (!ok) {
    throw InvalidInputException("metaindex %d rows aren't sorted by firstItem",
                                dst.size() - dst_len);
  }
}

} // namespace mergekv