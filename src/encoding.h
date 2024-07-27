#pragma once

#include "types.h"
#include <cstddef>

namespace mergekv {

enum MarshalType {
  marshalTypePlain,
  marshalTypeSZTD,
};

size_t CommonPrefixLen(const bytes &a, const bytes &b);
size_t CommonPrefixLen(const bytes_const_span a, const bytes_const_span b);
bool HasPrefix(const bytes_const_span a, const bytes_const_span b);

} // namespace mergekv