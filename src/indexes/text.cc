/*
 * Copyright (c) 2025, ValkeySearch contributors
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "src/indexes/text.h"

namespace valkey_search::indexes {

Text::Text(const data_model::TextIndex& text_index_proto)
    : IndexBase(IndexerType::kText),
      text_impl_(std::make_unique<text::TextFieldIndex>(text_index_proto)) {}

absl::StatusOr<bool> Text::AddRecord(const InternedStringPtr& key,
                                     absl::string_view data) {
  absl::MutexLock lock(&index_mutex_);
  // TODO: Delegate to Allen's text implementation
  return text_impl_->AddRecord(key, data);
}

absl::StatusOr<bool> Text::RemoveRecord(const InternedStringPtr& key,
                                        DeletionType deletion_type) {
  absl::MutexLock lock(&index_mutex_);
  // TODO: Delegate to Allen's text implementation
  return text_impl_->RemoveRecord(key, deletion_type);
}

absl::StatusOr<bool> Text::ModifyRecord(const InternedStringPtr& key,
                                        absl::string_view data) {
  absl::MutexLock lock(&index_mutex_);
  // TODO: Delegate to Allen's text implementation
  return text_impl_->ModifyRecord(key, data);
}

int Text::RespondWithInfo(ValkeyModuleCtx* ctx) const {
  // TODO: Delegate to Allen's text implementation
  return text_impl_->RespondWithInfo(ctx);
}

bool Text::IsTracked(const InternedStringPtr& key) const {
  // TODO: Delegate to Allen's text implementation
  return text_impl_->IsTracked(key);
}

uint64_t Text::GetRecordCount() const {
  // TODO: Delegate to Allen's text implementation
  return text_impl_->GetRecordCount();
}

std::unique_ptr<data_model::Index> Text::ToProto() const {
  // TODO: Delegate to Allen's text implementation
  return text_impl_->ToProto();
}

InternedStringPtr Text::GetRawValue(const InternedStringPtr& key) const {
  // TODO: Implement with Allen's text index design
  static InternedStringPtr empty;
  return empty;
}

// EntriesFetcherIterator implementations
bool Text::EntriesFetcherIterator::Done() const {
  // TODO: Implement with Allen's text index design
  return true;
}

void Text::EntriesFetcherIterator::Next() {
  // TODO: Implement with Allen's text index design
}

const InternedStringPtr& Text::EntriesFetcherIterator::operator*() const {
  // TODO: Implement with Allen's text index design
  static InternedStringPtr empty;
  return empty;
}

// EntriesFetcher implementations
size_t Text::EntriesFetcher::Size() const {
  // TODO: Implement with Allen's text index design
  return 0;
}

std::unique_ptr<EntriesFetcherIteratorBase> Text::EntriesFetcher::Begin() {
  // TODO: Implement with Allen's text index design
  return std::make_unique<EntriesFetcherIterator>();
}

std::unique_ptr<Text::EntriesFetcher> Text::Search(
    const query::TextPredicate& predicate, bool negate) const {
  // TODO: Implement with Allen's text search design
  return std::make_unique<EntriesFetcher>();
}

}  // namespace valkey_search::indexes
