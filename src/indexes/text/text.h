#ifndef VALKEY_SEARCH_INDEXES_TEXT_TEXT_H_
#define VALKEY_SEARCH_INDEXES_TEXT_TEXT_H_

/*

External API for text subsystem

*/

#include <concepts>
#include <memory>
#include <optional>

#include "absl/container/flat_hash_map.h"
#include "absl/functional/any_invocable.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "src/indexes/index_base.h"
#include "src/index_schema.pb.h"
#include "src/rdb_serialization.h"
#include "src/utils/string_interning.h"
#include "vmsdk/src/valkey_module_api/valkey_module.h"

// Forward declaration
namespace valkey_search::indexes {
enum class DeletionType;
enum class IndexerType;
}

namespace valkey_search {
namespace text {

using Key = valkey_search::InternedStringPtr;
using Position = uint32_t;

using Byte = uint8_t;
using Char = uint32_t;

// Forward declarations for Allen's design
struct TextIndex;
struct Postings;
template<typename T, bool suffix> struct RadixTree;

struct TextFieldIndex {
  TextFieldIndex(const data_model::TextIndex& text_index_proto) {}
  ~TextFieldIndex() = default;

  absl::StatusOr<bool> AddRecord(const InternedStringPtr& key,
                                 absl::string_view data) {
    return false; // Placeholder - Allen's implementation to follow
  }
  absl::StatusOr<bool> RemoveRecord(const InternedStringPtr& key,
                                    indexes::DeletionType deletion_type) {
    return false; // Placeholder - Allen's implementation to follow
  }
  absl::StatusOr<bool> ModifyRecord(const InternedStringPtr& key,
                                    absl::string_view data) {
    return false; // Placeholder - Allen's implementation to follow
  }
  int RespondWithInfo(ValkeyModuleCtx* ctx) const {
    return 0; // Placeholder - Allen's implementation to follow
  }
  bool IsTracked(const InternedStringPtr& key) const {
    return false; // Placeholder - Allen's implementation to follow
  }
  absl::Status SaveIndex(RDBChunkOutputStream chunked_out) const {
    return absl::OkStatus(); // Placeholder - Allen's implementation to follow
  }

  std::unique_ptr<data_model::Index> ToProto() const {
    auto index_proto = std::make_unique<data_model::Index>();
    index_proto->mutable_text_index();
    return index_proto;
  }
  void ForEachTrackedKey(
      absl::AnyInvocable<void(const InternedStringPtr&)> fn) const {
    // Placeholder - Allen's implementation to follow
  }

  uint64_t GetRecordCount() const {
    return 0; // Placeholder - Allen's implementation to follow
  }

 private:
  // Each text field is assigned a unique number within the containing index, this is used
  // by the Postings object to identify fields.
  size_t text_field_number = 0;
  // The per-index text index.
  std::shared_ptr<TextIndex> text_;
};

struct TextIndex {
  //
  // The main query data structure maps Words into Postings objects. This
  // is always done with a prefix tree. Optionally, a suffix tree can also be maintained.
  // But in any case for the same word the two trees must point to the same Postings object,
  // which is owned by this pair of trees. Plus, updates to these two trees need
  // to be atomic when viewed externally. The locking provided by the RadixTree object
  // is NOT quite sufficient to guarantee that the two trees are always in lock step.
  // thus this object becomes responsible for cross-tree locking issues.
  // Multiple locking strategies are possible. TBD (a shared-ed word lock table should work well)
  //
  std::shared_ptr<RadixTree<std::unique_ptr<Postings *>, false>> prefix_;
  std::optional<std::shared_ptr<RadixTree<Postings *, true>>> suffix_;
};

//
// this is a logical extension of the index-schema. could easily be merged into that object.
//
struct IndexSchemaText {
  //
  // This is the main index of all Text fields in this index schema
  //
  TextIndex corpus_;
  //
  // To support the Delete record and the post-filtering case, there is a separate
  // table of postings that are indexed by Key.
  //
  // This object must also ensure that updates of this object are multi-thread safe.
  //
  absl::flat_hash_map<Key, TextIndex> by_key_;
};

}  // namespace text
}  // namespace valkey_search

#endif
