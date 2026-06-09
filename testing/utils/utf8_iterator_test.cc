/*
 * Copyright (c) 2025, valkey-search contributors
 * All rights reserved.
 * SPDX-License-Identifier: BSD 3-Clause
 */

#include "src/utils/utf8_iterator.h"

#include <string>
#include <vector>

#include "gtest/gtest.h"

namespace valkey_search {
namespace utils {

// Helper: decode entire string into vector of codepoints using Next().
static std::vector<uint32_t> Decode(std::string_view s) {
  std::vector<uint32_t> result;
  Utf8Iterator it(s);
  while (it.Next()) {
    result.push_back(it.codepoint());
  }
  return result;
}

// ── Empty string ───────────────────────────────────────────────────────────

TEST(Utf8IteratorTest, EmptyStringReturnsNothingOnNext) {
  Utf8Iterator it("");
  EXPECT_FALSE(it.Next());
}

TEST(Utf8IteratorTest, CodePointCountEmptyIsZero) {
  EXPECT_EQ(0u, Utf8Iterator::CodePointCount(""));
}

// ── ASCII ──────────────────────────────────────────────────────────────────

TEST(Utf8IteratorTest, AsciiNullByte) {
  // U+0000 is valid UTF-8 encoded as a single 0x00 byte.
  std::string s(1, '\0');
  Utf8Iterator it(s);
  ASSERT_TRUE(it.Next());
  EXPECT_EQ(0u, it.codepoint());
  EXPECT_EQ(1u, it.byte_len());
  EXPECT_FALSE(it.Next());
}

TEST(Utf8IteratorTest, AsciiAllPrintable) {
  // Every ASCII byte 0x20..0x7E decodes as a single-byte code point.
  for (int i = 0x20; i <= 0x7E; ++i) {
    std::string s(1, static_cast<char>(i));
    Utf8Iterator it(s);
    ASSERT_TRUE(it.Next()) << "char 0x" << std::hex << i;
    EXPECT_EQ(static_cast<uint32_t>(i), it.codepoint()) << "char 0x" << i;
    EXPECT_EQ(1u, it.byte_len()) << "char 0x" << i;
    EXPECT_FALSE(it.Next());
  }
}

TEST(Utf8IteratorTest, AsciiMultipleChars) {
  auto v = Decode("hello");
  ASSERT_EQ(5u, v.size());
  EXPECT_EQ('h', v[0]);
  EXPECT_EQ('e', v[1]);
  EXPECT_EQ('l', v[2]);
  EXPECT_EQ('l', v[3]);
  EXPECT_EQ('o', v[4]);
}

TEST(Utf8IteratorTest, AsciiIsAsciiHelper) {
  EXPECT_TRUE(Utf8Iterator::IsAscii(0x00));
  EXPECT_TRUE(Utf8Iterator::IsAscii(0x7F));
  EXPECT_FALSE(Utf8Iterator::IsAscii(0x80));
  EXPECT_FALSE(Utf8Iterator::IsAscii(0xFF));
}

// ── Table-driven decode tests ──────────────────────────────────────────────
// Each test case encodes a valid UTF-8 sequence and the expected code points.

struct DecodeTestCase {
  std::string name;
  std::string input;
  std::vector<uint32_t> expected_codepoints;
  std::vector<uint8_t> expected_byte_lens;
};

class Utf8DecodeTest : public testing::TestWithParam<DecodeTestCase> {};

TEST_P(Utf8DecodeTest, DecodesCorrectly) {
  const auto& tc = GetParam();
  Utf8Iterator it(tc.input);
  for (size_t i = 0; i < tc.expected_codepoints.size(); ++i) {
    ASSERT_TRUE(it.Next()) << "stopped early at index " << i;
    EXPECT_EQ(tc.expected_codepoints[i], it.codepoint()) << "index " << i;
    EXPECT_EQ(tc.expected_byte_lens[i], it.byte_len()) << "index " << i;
  }
  EXPECT_FALSE(it.Next()) << "iterator not exhausted";
}

INSTANTIATE_TEST_SUITE_P(
    ValidSequences, Utf8DecodeTest,
    testing::Values(
        // 2-byte sequences
        DecodeTestCase{"TwoByteEAcute", "\xC3\xA9", {0x00E9u}, {2}},
        DecodeTestCase{"TwoByteBoundaryU0080", "\xC2\x80", {0x0080u}, {2}},
        DecodeTestCase{"TwoByteBoundaryU07FF", "\xDF\xBF", {0x07FFu}, {2}},
        // 3-byte sequences
        DecodeTestCase{"ThreeByteEuroSign", "\xE2\x82\xAC", {0x20ACu}, {3}},
        DecodeTestCase{
            "ThreeByteBoundaryU0800", "\xE0\xA0\x80", {0x0800u}, {3}},
        DecodeTestCase{
            "ThreeByteBoundaryUFFFF", "\xEF\xBF\xBF", {0xFFFFu}, {3}},
        // 4-byte sequences
        DecodeTestCase{"FourByteU10000", "\xF0\x90\x80\x80", {0x10000u}, {4}},
        DecodeTestCase{
            "FourByteMaxU10FFFF", "\xF4\x8F\xBF\xBF", {0x10FFFFu}, {4}},
        // Mixed sequences
        DecodeTestCase{"MixedAsciiAndTwoByte",
                       "abc\xC3\xA9",
                       {'a', 'b', 'c', 0x00E9u},
                       {1, 1, 1, 2}},
        DecodeTestCase{"MixedAllWidths",
                       "a\xC3\xA9\xE2\x82\xAC\xF0\x90\x80\x80",
                       {'a', 0x00E9u, 0x20ACu, 0x10000u},
                       {1, 2, 3, 4}}),
    [](const testing::TestParamInfo<DecodeTestCase>& info) {
      return info.param.name;
    });

// ── pos() tracking ─────────────────────────────────────────────────────────

TEST(Utf8IteratorTest, PosAdvancesCorrectly) {
  std::string s = "a\xC3\xA9z";  // a (1) + é (2) + z (1) = 4 bytes
  Utf8Iterator it(s);
  EXPECT_EQ(0u, it.pos());
  ASSERT_TRUE(it.Next());  // 'a'
  EXPECT_EQ(1u, it.pos());
  ASSERT_TRUE(it.Next());  // 'é'
  EXPECT_EQ(3u, it.pos());
  ASSERT_TRUE(it.Next());  // 'z'
  EXPECT_EQ(4u, it.pos());
  EXPECT_FALSE(it.Next());
  EXPECT_EQ(4u, it.pos());
}

TEST(Utf8IteratorTest, SingleByteStringExhaustsAfterOneNext) {
  Utf8Iterator it("x");
  ASSERT_TRUE(it.Next());
  EXPECT_EQ('x', it.codepoint());
  EXPECT_FALSE(it.Next());
  EXPECT_FALSE(it.Next());  // idempotent once exhausted
}

// ── CodePointCount ─────────────────────────────────────────────────────────

TEST(Utf8IteratorTest, CodePointCountAscii) {
  EXPECT_EQ(5u, Utf8Iterator::CodePointCount("hello"));
}

TEST(Utf8IteratorTest, CodePointCountMixed) {
  // "été" = U+00E9, t, U+00E9 = 3 code points, 5 bytes
  std::string s = "\xC3\xA9t\xC3\xA9";
  EXPECT_EQ(3u, Utf8Iterator::CodePointCount(s));
}

TEST(Utf8IteratorTest, CodePointCountThreeByte) {
  // U+20AC € = 1 code point, 3 bytes
  EXPECT_EQ(1u, Utf8Iterator::CodePointCount("\xE2\x82\xAC"));
}

TEST(Utf8IteratorTest, CodePointCountFourByte) {
  // U+10000 = 1 code point, 4 bytes
  EXPECT_EQ(1u, Utf8Iterator::CodePointCount("\xF0\x90\x80\x80"));
}

// ── AtLeastNCodepoints ─────────────────────────────────────────────────────

TEST(Utf8IteratorTest, AtLeastNCodepointsZeroAlwaysTrue) {
  EXPECT_TRUE(Utf8Iterator::AtLeastNCodepoints("", 0));
  EXPECT_TRUE(Utf8Iterator::AtLeastNCodepoints("hello", 0));
  EXPECT_TRUE(Utf8Iterator::AtLeastNCodepoints("\xC3\xA9", 0));
}

TEST(Utf8IteratorTest, AtLeastNCodepointsEmptyStringFalseForPositiveN) {
  EXPECT_FALSE(Utf8Iterator::AtLeastNCodepoints("", 1));
  EXPECT_FALSE(Utf8Iterator::AtLeastNCodepoints("", 100));
}

TEST(Utf8IteratorTest, AtLeastNCodepointsAsciiExactBoundary) {
  // "abc" has exactly 3 code points
  EXPECT_TRUE(Utf8Iterator::AtLeastNCodepoints("abc", 3));
  EXPECT_TRUE(Utf8Iterator::AtLeastNCodepoints("abc", 2));
  EXPECT_TRUE(Utf8Iterator::AtLeastNCodepoints("abc", 1));
  EXPECT_FALSE(Utf8Iterator::AtLeastNCodepoints("abc", 4));
  EXPECT_FALSE(Utf8Iterator::AtLeastNCodepoints("abc", 1000));
}

TEST(Utf8IteratorTest, AtLeastNCodepointsMultiByteExactBoundary) {
  // "été" = 3 code points, 5 bytes. Byte count would say >=5; codepoint
  // count says >=3 only.
  std::string s = "\xC3\xA9t\xC3\xA9";
  EXPECT_TRUE(Utf8Iterator::AtLeastNCodepoints(s, 3));
  EXPECT_TRUE(Utf8Iterator::AtLeastNCodepoints(s, 2));
  EXPECT_TRUE(Utf8Iterator::AtLeastNCodepoints(s, 1));
  EXPECT_FALSE(Utf8Iterator::AtLeastNCodepoints(s, 4));
  EXPECT_FALSE(Utf8Iterator::AtLeastNCodepoints(s, 5));
}

TEST(Utf8IteratorTest, AtLeastNCodepointsAgreesWithCodePointCount) {
  // For valid UTF-8, AtLeastNCodepoints(s, n) == (CodePointCount(s) >= n).
  const std::string cases[] = {
      "",
      "a",
      "hello world",
      "\xC3\xA9",                 // é, 2 bytes / 1 cp
      "\xC3\xA9t\xC3\xA9",        // été, 5 bytes / 3 cps
      "\xE2\x82\xAC",             // €, 3 bytes / 1 cp
      "\xF0\x90\x80\x80",         // U+10000, 4 bytes / 1 cp
      "abc\xC3\xA9\xE2\x82\xAC",  // mixed 1/2/3-byte
  };
  for (const auto& s : cases) {
    size_t cp_count = Utf8Iterator::CodePointCount(s);
    for (size_t n = 0; n <= cp_count + 2; ++n) {
      EXPECT_EQ(cp_count >= n, Utf8Iterator::AtLeastNCodepoints(s, n))
          << "input.size=" << s.size() << " cp_count=" << cp_count
          << " n=" << n;
    }
  }
}

// ── ExpectedLen ────────────────────────────────────────────────────────────

TEST(Utf8IteratorTest, ExpectedLenAscii) {
  for (int i = 0; i <= 0x7F; ++i) {
    EXPECT_EQ(1u, Utf8Iterator::ExpectedLen(static_cast<uint8_t>(i)));
  }
}

TEST(Utf8IteratorTest, ExpectedLenTwoByte) {
  // 0xC0..0xDF
  for (int i = 0xC0; i <= 0xDF; ++i) {
    EXPECT_EQ(2u, Utf8Iterator::ExpectedLen(static_cast<uint8_t>(i)));
  }
}

TEST(Utf8IteratorTest, ExpectedLenThreeByte) {
  // 0xE0..0xEF
  for (int i = 0xE0; i <= 0xEF; ++i) {
    EXPECT_EQ(3u, Utf8Iterator::ExpectedLen(static_cast<uint8_t>(i)));
  }
}

TEST(Utf8IteratorTest, ExpectedLenFourByte) {
  // Only 0xF0..0xF4 are valid 4-byte leads (U+10000..U+10FFFF).
  // 0xF5..0xF7 would encode code points > U+10FFFF and are invalid.
  for (int i = 0xF0; i <= 0xF4; ++i) {
    EXPECT_EQ(4u, Utf8Iterator::ExpectedLen(static_cast<uint8_t>(i)));
  }
}

TEST(Utf8IteratorTest, ExpectedLenInvalidReturnOne) {
  // 0x80..0xBF (continuation bytes) and 0xF5..0xFF (invalid leads) → 1
  for (int i = 0x80; i <= 0xBF; ++i) {
    EXPECT_EQ(1u, Utf8Iterator::ExpectedLen(static_cast<uint8_t>(i)));
  }
  // 0xF5..0xF7 encode > U+10FFFF (invalid), 0xF8..0xFF are always invalid.
  for (int i = 0xF5; i <= 0xFF; ++i) {
    EXPECT_EQ(1u, Utf8Iterator::ExpectedLen(static_cast<uint8_t>(i)));
  }
}

}  // namespace utils
}  // namespace valkey_search
