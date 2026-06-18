#pragma once
#include <string>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"

namespace valkey_search::indexes::text {

// Unicode normalization forms for multi-language text processing
enum class NormalizationForm {
  NFC,   // Canonical decomposition, then canonical composition
  NFKC,  // Compatibility decomposition, then canonical composition
  NFD,   // Canonical decomposition
  NFKD   // Compatibility decomposition
};

class UnicodeNormalizer {
 public:
  /// Performs Unicode case folding in-place on an existing string.
  /// Minimizes heap allocations by reusing the provided string's buffer.
  /// This is preferred for high-throughput tokenization loops.
  /// @param str The string to be modified; its capacity is reused while its
  /// content is replaced with the folded version.
  static void CaseFoldInPlace(std::string& str);

  /// Unicode normalization for consistent text comparison across languages.
  /// Uses ICU Normalizer2::normalizeUTF8 (UTF-8-native, no UnicodeString
  /// round-trip). Precondition: `text` is well-formed UTF-8 — the ICU API
  /// does not substitute U+FFFD for malformed input.
  /// @param text Input text to normalize (must be well-formed UTF-8)
  /// @param form Normalization form (NFC, NFKC, NFD, NFKD)
  /// @return Normalized text string
  static std::string Normalize(absl::string_view text, NormalizationForm form);

  /// Word boundary detection for CJK and complex script languages

  /// CJK) Handles Chinese, Japanese, Korean word segmentation without spaces
  /// @param text Input text for word segmentation
  /// @param locale Language locale (e.g., "zh", "ja", "ko", "" for auto-detect)
  /// @return Vector of word boundary positions
  /// @example FindWordBoundaries("北京大学", "zh") returns positions for "北京"
  /// + "大学"
  static std::vector<size_t> FindWordBoundaries(absl::string_view text,
                                                const std::string& locale = "");

  /// Locale-aware case folding for language-specific rules
  /// Handles special cases like Turkish i/İ distinction
  /// @param text Input text to fold
  /// @param locale Language locale for locale-specific rules
  /// @return Locale-aware case-folded text
  /// @example LocaleAwareCaseFold("İSTANBUL", "tr") handles Turkish correctly
  static std::string LocaleAwareCaseFold(absl::string_view text,
                                         const std::string& locale);
};

}  // namespace valkey_search::indexes::text
