/*
 * Copyright (c) 2025, valkey-search contributors
 * All rights reserved.
 * SPDX-License-Identifier: BSD 3-Clause
 *
 */

#ifndef VALKEYSEARCH_SRC_INDEXES_TEXT_INDEX_H_
#define VALKEYSEARCH_SRC_INDEXES_TEXT_INDEX_H_

#include <memory>
#include <optional>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "src/utils/string_interning.h"

namespace valkey_search {
namespace text {

using Key = InternedStringPtr;
using Position = uint32_t;

// Forward declarations
struct Postings;

/**
 * RadixTree template for text indexing
 * 
 * This template provides a tree structure for indexing words in text documents
 * The suffix parameter determines whether this is a prefix or suffix tree
 */
template<typename T, bool suffix>
struct RadixTree {
  // Implementation will be provided later
};

/**
 * TextIndex holds the primary indexing structures for text search
 * 
 * It maintains both prefix and optional suffix trees for word lookups,
 * both pointing to the same Postings objects.
 */
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

  absl::flat_hash_map<Key, RadixTree<Postings*, true>> reverse_;

  // Tracks unindexed keys
  absl::flat_hash_set<Key> untracked_keys_;
};

/**
 * IndexSchemaText represents the text indexing for an entire schema
 * 
 * This allows for cross-field text operations and maintains both
 * global and key-specific indices.
 */
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

#endif  // VALKEYSEARCH_SRC_INDEXES_TEXT_INDEX_H_
