/*
 * Copyright Valkey Contributors.
 * All rights reserved.
 * SPDX-License-Identifier: BSD 3-Clause
 */

#ifndef VALKEYSEARCH_UTILS_SCANNER_H
#define VALKEYSEARCH_UTILS_SCANNER_H

#include <absl/log/check.h>

#include <cctype>
#include <cstdint>
#include <optional>

#include "absl/strings/string_view.h"

namespace valkey_search {
namespace utils {

class Scanner {
 public:
  using Char = uint32_t;

 private:
  // UTF-8 encoding/decoding bit patterns. Scanner has its own self-contained
  // multi-byte UTF-8 decoder in NextUtf8() because Scanner is the *validation*
  // layer: it must tolerate invalid bytes by counting them, not crash on them.
  // Utf8Iterator (used by post-validation callsites) has the opposite
  // contract — it CHECKs on invalid bytes — so Scanner cannot delegate to it.
  enum : uint32_t {
    kStart1Mask = 0b10000000,
    kStart1Value = 0b00000000,
    kStart2Mask = 0b11100000,
    kStart2Value = 0b11000000,
    kStart3Mask = 0b11110000,
    kStart3Value = 0b11100000,
    kStart4Mask = 0b11111000,
    kStart4Value = 0b11110000,
    kMoreMask = 0b11000000,
    kMoreValue = 0b10000000,
  };

  Char GetByte(size_t pos) const { return sv_[pos] & 0xFF; }

  bool IsStart(size_t mask, size_t value) const {
    return (GetByte(pos_) & mask) == value;
  }

  Char GetStart(size_t mask) { return GetByte(pos_++) & ~mask; }

  bool IsMore(size_t pos) const {
    return pos < sv_.size() && ((GetByte(pos) & kMoreMask) == kMoreValue);
  }

  Char GetMore(char32_t result) {
    return (result << 6) | (GetByte(pos_++) & ~kMoreMask);
  }

 public:
  Scanner(absl::string_view sv) : sv_(sv) {}
  size_t GetPosition() const { return pos_; }

  static constexpr Char kEOF = (Char)-1;
  static constexpr Char kMaxCodepoint = 0x10FFFF;

  Char PeekByte() {
    if (pos_ >= sv_.size()) {
      return kEOF;
    } else {
      return static_cast<uint8_t>(sv_[pos_]);
    }
  }

  Char NextByte() {
    if (pos_ >= sv_.size()) {
      return kEOF;
    } else {
      return static_cast<uint8_t>(sv_[pos_++]);
    }
  }

  bool PopByte(Char c) {
    CHECK(c != kEOF);
    if (PeekByte() == c) {
      pos_++;
      return true;
    } else {
      return false;
    }
  }

  // Decode and advance one UTF-8 code point. Self-contained multi-byte
  // decoder that *tolerates* invalid bytes (counts them via
  // invalid_utf_count_) — Scanner is the validation layer used by
  // IsValidUtf8(); it must never crash on malformed input.
  Char NextUtf8() {
    if (pos_ >= sv_.size()) {
      return kEOF;
    }
    if (IsStart(kStart1Mask, kStart1Value)) {
      return GetStart(kStart1Mask);
    } else if (IsStart(kStart2Mask, kStart2Value) && IsMore(pos_ + 1)) {
      return GetMore(GetStart(kStart2Mask));
    } else if (IsStart(kStart3Mask, kStart3Value) && IsMore(pos_ + 1) &&
               IsMore(pos_ + 2)) {
      return GetMore(GetMore(GetStart(kStart3Mask)));
    } else if (IsStart(kStart4Mask, kStart4Value) && IsMore(pos_ + 1) &&
               IsMore(pos_ + 2) && IsMore(pos_ + 3)) {
      return GetMore(GetMore(GetMore(GetStart(kStart4Mask))));
    }
    invalid_utf_count_++;
    return GetByte(pos_++) & 0xFF;
  }

  Char PeekUtf8() {
    size_t pos = pos_;
    Char result = NextUtf8();
    pos_ = pos;
    return result;
  }

  void SkipWhiteSpace() {
    while (std::isspace(PeekByte())) {
      (void)NextByte();
    }
  }

  //
  // These routines transparently skip whitespace and automatically handle utf-8
  //
  int SkipWhiteSpacePeekByte() {
    size_t pos = pos_;
    SkipWhiteSpace();
    auto result = PeekByte();
    pos_ = pos;
    return result;
  }

  int SkipWhiteSpaceNextByte() {
    SkipWhiteSpace();
    return NextByte();
  }

  bool SkipWhiteSpacePopByte(Char c) {
    size_t pos = pos_;
    SkipWhiteSpace();
    if (PopByte(c)) {
      return true;
    } else {
      pos_ = pos;
      return false;
    }
  }

  bool SkipWhiteSpacePopWord(absl::string_view word) {
    size_t pos = pos_;
    SkipWhiteSpace();
    for (auto ch : word) {
      if (NextByte() != ch) {
        pos_ = pos;
        return false;
      }
    }
    return true;
  }

  std::optional<double> PopDouble() {
    if (pos_ >= sv_.size()) {
      return std::nullopt;
    }
    double d = 0.0;
#if 0
    // CLANG doesn't support from_chars for floating point types.
    auto [ptr, ec] = std::from_chars(&sv_[pos_], sv_.data() + sv_.size(), d);
    if (ec == std::errc::invalid_argument) {
      return std::nullopt;
    }
    CHECK(ec == std::errc());
    pos_ = ptr - sv_.data();
    CHECK(pos_ <= sv_.size());
#else
    absl::string_view s(sv_);
    s.remove_prefix(pos_);
    std::string null_terminated(s);
    char* scanned{nullptr};
    d = std::strtod(null_terminated.data(), &scanned);
    if (scanned == null_terminated.data()) {
      return std::nullopt;
    }
    pos_ += scanned - null_terminated.data();
    CHECK(pos_ <= sv_.size());
#endif
    return d;
  }

  absl::string_view GetUnscanned() const {
    auto copy = sv_;
    copy.remove_prefix(pos_);
    return copy;
  }

  absl::string_view GetScanned() const {
    auto copy = sv_;
    copy.remove_prefix(sv_.size() - pos_);
    return copy;
  }

  static std::string& PushBackUtf8(std::string& s, Scanner::Char codepoint) {
    if (codepoint <= 0x7F) {
      s += char(codepoint);
    } else if (codepoint <= 0x7FF) {
      s += char(kStart2Value | (codepoint >> 6));
      s += char(kMoreValue | (codepoint & ~kMoreMask));
    } else if (codepoint <= 0xFFFF) {
      s += char(kStart3Value | (codepoint >> 12));
      s += char(kMoreValue | ((codepoint >> 6) & ~kMoreMask));
      s += char(kMoreValue | (codepoint & ~kMoreMask));
    } else if (codepoint <= 0x10FFFF) {
      s += char(kStart4Value | (codepoint >> 18));
      s += char(kMoreValue | ((codepoint >> 12) & ~kMoreMask));
      s += char(kMoreValue | ((codepoint >> 6) & ~kMoreMask));
      s += char(kMoreValue | (codepoint & ~kMoreMask));
    } else {
      // std::cerr << "Found invalid codepoint " << codepoint << "(" << std::hex
      // << size_t(codepoint) << ")\n";
      CHECK(false);
    }
    return s;
  }

  size_t GetInvalidUtf8Count() const { return invalid_utf_count_; }

 private:
  absl::string_view sv_;
  size_t pos_{0};
  size_t invalid_utf_count_{0};
};

}  // namespace utils
}  // namespace valkey_search

#endif
