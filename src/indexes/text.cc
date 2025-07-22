/*
 * Copyright (c) 2025, valkey-search contributors
 * All rights reserved.
 * SPDX-License-Identifier: BSD 3-Clause
 *
 */

#include "src/indexes/text.h"
#include "src/indexes/text_index.h"

namespace valkey_search::indexes {

Text::Text(const data_model::TextIndex& text_index_proto, 
           const data_model::IndexSchema* index_schema_proto)
    : IndexBase(IndexerType::kText),
      text_impl_(std::make_unique<text::TextFieldIndex>(
          text_index_proto,
          index_schema_proto ? index_schema_proto->punctuation() : "",
          index_schema_proto ? index_schema_proto->with_offsets() : true,
          index_schema_proto ? std::vector<std::string>(index_schema_proto->stop_words().begin(),
                                                       index_schema_proto->stop_words().end()) : 
                              std::vector<std::string>(),
          index_schema_proto ? index_schema_proto->language() : data_model::LANGUAGE_ENGLISH,
          index_schema_proto ? index_schema_proto->nostem() : false,
          index_schema_proto ? index_schema_proto->min_stem_size() : 4)) {}

absl::StatusOr<bool> Text::AddRecord(const InternedStringPtr& key,
                                     absl::string_view data) {
  absl::MutexLock lock(&index_mutex_);
  // TODO: stub
  return text_impl_->AddRecord(key, data);
}

absl::StatusOr<bool> Text::RemoveRecord(const InternedStringPtr& key,
                                        DeletionType deletion_type) {
  absl::MutexLock lock(&index_mutex_);
  // TODO: stub
  return text_impl_->RemoveRecord(key, deletion_type);
}

absl::StatusOr<bool> Text::ModifyRecord(const InternedStringPtr& key,
                                        absl::string_view data) {
  absl::MutexLock lock(&index_mutex_);
  // TODO: stub
  return text_impl_->ModifyRecord(key, data);
}

int Text::RespondWithInfo(ValkeyModuleCtx* ctx) const {
  // TODO: stub
  return text_impl_->RespondWithInfo(ctx);
}

bool Text::IsTracked(const InternedStringPtr& key) const {
  // TODO: stub
  return text_impl_->IsTracked(key);
}

uint64_t Text::GetRecordCount() const {
  // TODO: stub
  return text_impl_->GetRecordCount();
}

std::unique_ptr<data_model::Index> Text::ToProto() const {
  // TODO: stub
  return text_impl_->ToProto();
}

InternedStringPtr Text::GetRawValue(const InternedStringPtr& key) const {
  // TODO:stub
  static InternedStringPtr empty;
  return empty;
}

// EntriesFetcherIterator implementations
bool Text::EntriesFetcherIterator::Done() const {
  // TODO:stub
  return true;
}

void Text::EntriesFetcherIterator::Next() {
  // TODO:stub
}

const InternedStringPtr& Text::EntriesFetcherIterator::operator*() const {
  // TODO:stub
  static InternedStringPtr empty;
  return empty;
}

// EntriesFetcher implementations
size_t Text::EntriesFetcher::Size() const {
  // TODO:stub
  return 0;
}

std::unique_ptr<EntriesFetcherIteratorBase> Text::EntriesFetcher::Begin() {
  // TODO:stub
  return std::make_unique<EntriesFetcherIterator>();
}

std::unique_ptr<Text::EntriesFetcher> Text::Search(
    const query::TextPredicate& predicate, bool negate) const {
  // TODO: stub
  return std::make_unique<EntriesFetcher>();
}

}  // namespace valkey_search::indexes

namespace valkey_search {
namespace text {

absl::StatusOr<bool> TextFieldIndex::AddRecord(const InternedStringPtr& key,
                                              absl::string_view data) {
  // Stub implementation - will be replaced in the lexical scanner PR
  // Just return success to allow building and basic functionality
  return true;
}

}  // namespace text
}  // namespace valkey_search
