#include "inmemory_part.h"
#include "filenames.h"
#include "types.h"

namespace mergekv {

void PartHeader::MustReadMetadata(const string &part_path) {
  Reset();
  fs::path name = kMetadataFilename;
  fs::path base_path = part_path;

  fs::path metadata_path = base_path / name;
}

} // namespace mergekv