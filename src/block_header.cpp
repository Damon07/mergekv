#include "block_header.h"
#include "encoding_util.h"
#include "exception.h"
#include "inmemory_block.h"
#include "types.h"
#include <cstdint>
#include "encoding.h"
#include "string_util.h"

namespace mergekv {



void BlockHeader::Reset() {
    if (no_copy) {
        common_prefix = bytes_span();
        first_item = bytes_span();
    } else {
        common_prefix.subspan(0,0);
        first_item.subspan(0,0);
    }
    items_count = 0;
    items_block_offset = 0;
    lens_block_offset = 0;
    items_block_size = 0;
    lens_block_size = 0;
    mt = marshalTypePlain;
}

void BlockHeader::Marshal(bytes &dst) {
    EncodingUtil::MarshalBytes(dst, common_prefix); 
    EncodingUtil::MarshalBytes(dst, first_item);
    dst.push_back(static_cast<uint8_t>(mt));
    EncodingUtil::MarshalUint32(dst, items_count);
    EncodingUtil::MarshalUint64(dst, items_block_offset);
    EncodingUtil::MarshalUint64(dst, lens_block_offset);
    EncodingUtil::MarshalUint64(dst, items_block_size);
    EncodingUtil::MarshalUint64(dst, lens_block_size);
}

// UnmarshalNoCopy unmarshals bh from src without copying the data from src.
//
// The src must remain unchanged while bh is in use.
bytes_const_span BlockHeader::UnmarshalNoCopy(bytes_const_span src) {
    no_copy = true;

    auto [cp, n_size] = EncodingUtil::UnmarshalBytes(src);
    if (n_size <= 0) {
        throw InvalidInputException("cannot unmarshal commonPrefix");
    }

    src = src.subspan(n_size);
    common_prefix = cp;

    auto [fi, _n_size] = EncodingUtil::UnmarshalBytes(src);
    if (n_size <= 0) {
        throw InvalidInputException("cannot unmarshal firstItem");
    }

    src = src.subspan(_n_size);
    first_item = fi;

    if (src.size() < 1) {
        throw InvalidInputException("cannot unmarshal marshalType");
    }

    mt = static_cast<MarshalType>(src[0]);
    src = src.subspan(1);
    if (!check_marshal_type(mt)) {
        throw InvalidInputException("invalid marshalType");
    }

    if (src.size() < 4) {
        throw InvalidInputException("cannot unmarshal itemsCount");
    }

    items_count = EncodingUtil::UnmarshalUint32(src);
    src = src.subspan(4);
    if (src.size() < 8) {
        throw InvalidInputException("cannot unmarshal itemsBlockOffset");
    }
    items_block_offset = EncodingUtil::UnmarshalUint64(src);
    src = src.subspan(8);
    if (src.size() < 8) {
        throw InvalidInputException("cannot unmarshal lensBlockOffset");
    }
    lens_block_offset = EncodingUtil::UnmarshalUint64(src);
    src = src.subspan(8);
    if (src.size() < 4) {
        throw InvalidInputException("cannot unmarshal itemsBlockSize");
    }
    items_block_size = EncodingUtil::UnmarshalUint32(src);
    src = src.subspan(4);

    if (src.size() < 4) {
        throw InvalidInputException("cannot unmarshal lensBlockSize");
    }
    lens_block_size = EncodingUtil::UnmarshalUint32(src);
    src = src.subspan(4);

    if (items_count <= 0) {
        throw InvalidInputException("invalid itemsCount");
    }

    if (items_block_size > 2 * kMaxInmemoryBlockSize) {
        throw InvalidInputException("too big itemsBlockSize; got %d; cannot exceed %d", items_block_size, 2 * kMaxInmemoryBlockSize);
    }

    if (lens_block_size > 2 *8* kMaxInmemoryBlockSize) {
        throw InvalidInputException("too big lensBlockSize; got %d; cannot exceed %d", lens_block_size, 2*8*kMaxInmemoryBlockSize);
    }

    return src;
}

// unmarshalBlockHeadersNoCopy unmarshals all the block headers from src,
// appends them to dst and returns the appended result.
//
// Block headers must be sorted by bh.firstItem.
//
// It is expected that src remains unchanged while rhe returned blocks are in use.
void UnmarshalBHNoCopy(std::vector<BlockHeader> &dst, bytes_const_span src, int bh_count) {
    if (bh_count <= 0) {
        throw InvalidInputException("invalid bh_count");
    }

    auto dst_len = dst.size();
    dst.resize(dst_len + bh_count);
    for (int i = 0; i < bh_count; i++) {
        auto &bh = dst[dst_len + i];
        src = bh.UnmarshalNoCopy(src);
    }

    if (src.size() != 0) {
        throw InvalidInputException("unexpected non-zero tail left after unmarshaling %d block headers; len(tail)=%d", bh_count, src.size());
    }

    auto new_bhs = std::span<BlockHeader>(dst.data() + dst_len, bh_count);
    if (!std::is_sorted(new_bhs.begin(), new_bhs.end(), [](const BlockHeader &a, const BlockHeader &b) {
        return StringUtil::ToStringView(a.first_item) < StringUtil::ToStringView(b.first_item);
    })) {
        throw InvalidInputException("block headers are not sorted");
    }
}

}