#include "part_header.h"
#include "filenames.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <simdjson.h>

namespace mergekv {
void PartHeader::MustReadMetadata(const string &part_path) {
  Reset();
  fs::path base_path = part_path;
  auto metadata_path = fs::path(base_path / kMetadataFilename);

  std::ifstream file(metadata_path, std::ios::binary);
  if (!file.is_open()) {
    throw InvalidInputException("Failed to open file: " +
                                metadata_path.string());
  }
  file.seekg(0, std::ios::end);
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  bytes metadata(size);
  file.read(reinterpret_cast<char *>(metadata.data()), size);

  simdjson::dom::parser parser;
  simdjson::dom::element metadata_element;
  auto error =
      parser.parse(metadata.data(), metadata.size()).get(metadata_element);
  if (error) {
    throw InvalidInputException("Failed to parse metadata: error_code: %d",
                                error);
  }

  PartHeaderJson phj;
  if (metadata_element["items_count"].get(items_count_) != simdjson::SUCCESS) {
    throw InvalidInputException("Failed to parse 'items_count'");
  }
  if (metadata_element["blocks_count"].get(blocks_count_) !=
      simdjson::SUCCESS) {
    throw InvalidInputException("Failed to parse 'blocks_count'");
  }

  if (metadata_element["first_item"].get_string().get(phj.first_item) !=
      simdjson::SUCCESS) {
    throw InvalidInputException("Failed to parse 'first_item'");
  }

  if (metadata_element["last_item"].get_string().get(phj.last_item) !=
      simdjson::SUCCESS) {
    throw InvalidInputException("Failed to parse 'last_item'");
  }

  items_count_ = phj.items_count;
  if (items_count_ < 1) {
    throw InvalidInputException("items_count must be greater than 0");
  }
  blocks_count_ = phj.blocks_count;
  if (blocks_count_ < 1) {
    throw InvalidInputException("blocks_count must be greater than 0");
  }

  if (blocks_count_ > items_count_) {
    throw InvalidInputException(
        "blocks_count must be less than or equal to "
        "items_count: blocks_count: %d, items_count: %d",
        blocks_count_, items_count_);
  }

  first_item_ =
      StringUtil::DecodeHex(StringUtil::BytesConstSpan(phj.first_item));
  last_item_ = StringUtil::DecodeHex(StringUtil::BytesConstSpan(phj.last_item));
}

void PartHeader::MustWriteMetadata(const string &part_path) {
  fs::path base_path = part_path;
  auto metadata_path = fs::path(base_path / kMetadataFilename);

  nlohmann::json metadata_object;
  metadata_object["items_count"] = items_count_;
  metadata_object["blocks_count"] = blocks_count_;
  metadata_object["first_item"] = StringUtil::EncodeHex(first_item_);
  metadata_object["last_item"] = StringUtil::EncodeHex(last_item_);

  string metadata_data = metadata_object.dump();
  std::ofstream metadata_file(metadata_path);
  metadata_file << metadata_data;
}
} // namespace mergekv