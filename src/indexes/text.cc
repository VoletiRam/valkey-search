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
          index_schema_proto,
          // Pass field identifier if available, otherwise empty string
          (index_schema_proto && !index_schema_proto->name().empty()) 
              ? index_schema_proto->name() : "")) {}

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
