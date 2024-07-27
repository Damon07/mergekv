#include "encoding.h"
#include "exception.h"
#include <cstddef>
#include <cstdint>
#include <zstd.h>
#include <zstd_errors.h>

namespace mergekv {
size_t CommonPrefixLen(const bytes &a, const bytes &b) {
  size_t i = 0;
  if (a.size() > b.size()) {
    for (i = 0; i < b.size(); i++) {
      if (a[i] != b[i]) {
        break;
      }
    }
  } else {
    for (i = 0; i < a.size(); i++) {
      if (a[i] != b[i]) {
        break;
      }
    }
  }
  return i;
}

size_t CommonPrefixLen(const bytes_const_span a, const bytes_const_span b) {
  size_t i = 0;
  if (a.size() > b.size()) {
    for (i = 0; i < b.size(); i++) {
      if (a[i] != b[i]) {
        break;
      }
    }
  } else {
    for (i = 0; i < a.size(); i++) {
      if (a[i] != b[i]) {
        break;
      }
    }
  }
  return i;
}
bool HasPrefix(const bytes_const_span a, const bytes_const_span b) {
  // 检查长度
  if (a.size() > b.size())
    return false;

  // 逐字节比较
  for (size_t i = 0; i < a.size(); ++i) {
    if (a[i] != b[i])
      return false;
  }

  return true;
}

} // namespace mergekv