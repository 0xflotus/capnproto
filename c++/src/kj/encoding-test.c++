// Copyright (c) 2017 Cloudflare, Inc. and contributors
// Licensed under the MIT License:
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "encoding.h"
#include <kj/test.h>
#include <stdint.h>

namespace kj {
namespace {

CappedArray<char, sizeof(char    ) * 2 + 1> hex(char     i) { return kj::hex((uint8_t )i); }
CappedArray<char, sizeof(char16_t) * 2 + 1> hex(char16_t i) { return kj::hex((uint16_t)i); }
CappedArray<char, sizeof(char32_t) * 2 + 1> hex(char32_t i) { return kj::hex((uint32_t)i); }
// Hexify chars correctly.
//
// TODO(cleanup): Should this go into string.h with the other definitions of hex()?

template <typename T>
void expectUtf(UtfResult<T> result,
               ArrayPtr<const Decay<decltype(result[0])>> expected,
               bool errors = false) {
  if (errors) {
    KJ_EXPECT(result.hadErrors);
  } else {
    KJ_EXPECT(!result.hadErrors);
  }

  KJ_EXPECT(result.size() == expected.size(), result.size(), expected.size());
  for (auto i: kj::zeroTo(kj::min(result.size(), expected.size()))) {
    KJ_EXPECT(result[i] == expected[i], i, hex(result[i]), hex(expected[i]));
  }
}

template <typename T, size_t s>
void expectUtf(UtfResult<T> result,
               const Decay<decltype(result[0])> (&expected)[s],
               bool errors = false) {
  expectUtf(kj::mv(result), arrayPtr(expected, s - 1), errors);
}

KJ_TEST("encode UTF-8 to UTF-16") {
  expectUtf(encodeUtf16(u8"foo"), u"foo");
  expectUtf(encodeUtf16(u8"Здравствуйте"), u"Здравствуйте");
  expectUtf(encodeUtf16(u8"中国网络"), u"中国网络");
  expectUtf(encodeUtf16(u8"😺☁☄🐵"), u"😺☁☄🐵");
}

KJ_TEST("invalid UTF-8 to UTF-16") {
  // Disembodied continuation byte.
  expectUtf(encodeUtf16("\x80"), u"\ufffd", true);
  expectUtf(encodeUtf16("f\xbfo"), u"f\ufffdo", true);
  expectUtf(encodeUtf16("f\xbf\x80\xb0o"), u"f\ufffdo", true);

  // Missing continuation bytes.
  expectUtf(encodeUtf16("\xc2x"), u"\ufffdx", true);
  expectUtf(encodeUtf16("\xe0x"), u"\ufffdx", true);
  expectUtf(encodeUtf16("\xe0\xa0x"), u"\ufffdx", true);
  expectUtf(encodeUtf16("\xf0x"), u"\ufffdx", true);
  expectUtf(encodeUtf16("\xf0\x90x"), u"\ufffdx", true);
  expectUtf(encodeUtf16("\xf0\x90\x80x"), u"\ufffdx", true);

  // Overlong sequences.
  expectUtf(encodeUtf16("\xc0\x80"), u"\ufffd", true);
  expectUtf(encodeUtf16("\xc1\xbf"), u"\ufffd", true);
  expectUtf(encodeUtf16("\xc2\x80"), u"\u0080", false);
  expectUtf(encodeUtf16("\xdf\xbf"), u"\u07ff", false);

  expectUtf(encodeUtf16("\xe0\x80\x80"), u"\ufffd", true);
  expectUtf(encodeUtf16("\xe0\x9f\xbf"), u"\ufffd", true);
  expectUtf(encodeUtf16("\xe0\xa0\x80"), u"\u0800", false);
  expectUtf(encodeUtf16("\xef\xbf\xbf"), u"\uffff", false);

  expectUtf(encodeUtf16("\xf0\x80\x80\x80"), u"\ufffd", true);
  expectUtf(encodeUtf16("\xf0\x8f\xbf\xbf"), u"\ufffd", true);
  expectUtf(encodeUtf16("\xf0\x90\x80\x80"), u"\U00010000", false);
  expectUtf(encodeUtf16("\xf4\x8f\xbf\xbf"), u"\U0010ffff", false);

  // Out of Unicode range.
  expectUtf(encodeUtf16("\xf5\x80\x80\x80"), u"\ufffd", true);
  expectUtf(encodeUtf16("\xf8\xbf\x80\x80\x80"), u"\ufffd", true);
  expectUtf(encodeUtf16("\xfc\xbf\x80\x80\x80\x80"), u"\ufffd", true);
  expectUtf(encodeUtf16("\xfe\xbf\x80\x80\x80\x80\x80"), u"\ufffd", true);
  expectUtf(encodeUtf16("\xff\xbf\x80\x80\x80\x80\x80\x80"), u"\ufffd", true);
}

KJ_TEST("encode UTF-8 to UTF-32") {
  expectUtf(encodeUtf32(u8"foo"), U"foo");
  expectUtf(encodeUtf32(u8"Здравствуйте"), U"Здравствуйте");
  expectUtf(encodeUtf32(u8"中国网络"), U"中国网络");
  expectUtf(encodeUtf32(u8"😺☁☄🐵"), U"😺☁☄🐵");
}

KJ_TEST("invalid UTF-8 to UTF-32") {
  // Disembodied continuation byte.
  expectUtf(encodeUtf32("\x80"), U"\ufffd", true);
  expectUtf(encodeUtf32("f\xbfo"), U"f\ufffdo", true);
  expectUtf(encodeUtf32("f\xbf\x80\xb0o"), U"f\ufffdo", true);

  // Missing continuation bytes.
  expectUtf(encodeUtf32("\xc2x"), U"\ufffdx", true);
  expectUtf(encodeUtf32("\xe0x"), U"\ufffdx", true);
  expectUtf(encodeUtf32("\xe0\xa0x"), U"\ufffdx", true);
  expectUtf(encodeUtf32("\xf0x"), U"\ufffdx", true);
  expectUtf(encodeUtf32("\xf0\x90x"), U"\ufffdx", true);
  expectUtf(encodeUtf32("\xf0\x90\x80x"), U"\ufffdx", true);

  // Overlong sequences.
  expectUtf(encodeUtf32("\xc0\x80"), U"\ufffd", true);
  expectUtf(encodeUtf32("\xc1\xbf"), U"\ufffd", true);
  expectUtf(encodeUtf32("\xc2\x80"), U"\u0080", false);
  expectUtf(encodeUtf32("\xdf\xbf"), U"\u07ff", false);

  expectUtf(encodeUtf32("\xe0\x80\x80"), U"\ufffd", true);
  expectUtf(encodeUtf32("\xe0\x9f\xbf"), U"\ufffd", true);
  expectUtf(encodeUtf32("\xe0\xa0\x80"), U"\u0800", false);
  expectUtf(encodeUtf32("\xef\xbf\xbf"), U"\uffff", false);

  expectUtf(encodeUtf32("\xf0\x80\x80\x80"), U"\ufffd", true);
  expectUtf(encodeUtf32("\xf0\x8f\xbf\xbf"), U"\ufffd", true);
  expectUtf(encodeUtf32("\xf0\x90\x80\x80"), U"\U00010000", false);
  expectUtf(encodeUtf32("\xf4\x8f\xbf\xbf"), U"\U0010ffff", false);

  // Out of Unicode range.
  expectUtf(encodeUtf32("\xf5\x80\x80\x80"), U"\ufffd", true);
  expectUtf(encodeUtf32("\xf8\xbf\x80\x80\x80"), U"\ufffd", true);
  expectUtf(encodeUtf32("\xfc\xbf\x80\x80\x80\x80"), U"\ufffd", true);
  expectUtf(encodeUtf32("\xfe\xbf\x80\x80\x80\x80\x80"), U"\ufffd", true);
  expectUtf(encodeUtf32("\xff\xbf\x80\x80\x80\x80\x80\x80"), U"\ufffd", true);
}

KJ_TEST("decode UTF-16 to UTF-8") {
  expectUtf(decodeUtf16(u"foo"), u8"foo");
  expectUtf(decodeUtf16(u"Здравствуйте"), u8"Здравствуйте");
  expectUtf(decodeUtf16(u"中国网络"), u8"中国网络");
  expectUtf(decodeUtf16(u"😺☁☄🐵"), u8"😺☁☄🐵");
}

KJ_TEST("invalid UTF-16 to UTF-8") {
  // Surrogates in wrong order.
  expectUtf(decodeUtf16(u"\xd7ff\xdc00\xdfff\xe000"), u8"\ud7ff\ufffd\ufffd\ue000", true);

  // Missing second surrogate.
  expectUtf(decodeUtf16(u"f\xd800"), u8"f\ufffd", true);
  expectUtf(decodeUtf16(u"f\xd800x"), u8"f\ufffdx", true);
  expectUtf(decodeUtf16(u"f\xd800\xd800x"), u8"f\ufffd\ufffdx", true);
}

KJ_TEST("decode UTF-32 to UTF-8") {
  expectUtf(decodeUtf32(U"foo"), u8"foo");
  expectUtf(decodeUtf32(U"Здравствуйте"), u8"Здравствуйте");
  expectUtf(decodeUtf32(U"中国网络"), u8"中国网络");
  expectUtf(decodeUtf32(U"😺☁☄🐵"), u8"😺☁☄🐵");
}

KJ_TEST("invalid UTF-32 to UTF-8") {
  // Surrogates rejected.
  expectUtf(decodeUtf32(U"\xd7ff\xdc00\xdfff\xe000"), u8"\ud7ff\ufffd\ufffd\ue000", true);

  // Even if it would be a valid surrogate pair in UTF-16.
  expectUtf(decodeUtf32(U"\xd7ff\xd800\xdfff\xe000"), u8"\ud7ff\ufffd\ufffd\ue000", true);
}

KJ_TEST("tryEncode / tryDecode") {
  KJ_EXPECT(tryEncodeUtf16("\x80") == nullptr);
  KJ_EXPECT(ArrayPtr<const char16_t>(KJ_ASSERT_NONNULL(tryEncodeUtf16("foo")))
         == arrayPtr(u"foo", 3));

  KJ_EXPECT(tryEncodeUtf32("\x80") == nullptr);
  KJ_EXPECT(ArrayPtr<const char32_t>(KJ_ASSERT_NONNULL(tryEncodeUtf32("foo")))
         == arrayPtr(U"foo", 3));

  KJ_EXPECT(tryDecodeUtf16(u"\xd800") == nullptr);
  KJ_EXPECT(KJ_ASSERT_NONNULL(tryDecodeUtf16(u"foo")) == "foo");
  KJ_EXPECT(tryDecodeUtf32(U"\xd800") == nullptr);
  KJ_EXPECT(KJ_ASSERT_NONNULL(tryDecodeUtf32(U"foo")) == "foo");
}

// =======================================================================================

KJ_TEST("hex encoding/decoding") {
  byte bytes[] = {0x12, 0x34, 0xab, 0xf2};

  KJ_EXPECT(encodeHex(bytes) == "1234abf2");
  KJ_EXPECT(decodeHex("1234abf2").asPtr() == bytes);
}

KJ_TEST("URI encoding/decoding") {
  KJ_EXPECT(encodeUriComponent("foo") == "foo");
  KJ_EXPECT(encodeUriComponent("foo bar") == "foo%20bar");
  KJ_EXPECT(encodeUriComponent("\xab\xba") == "%ab%ba");
  KJ_EXPECT(encodeUriComponent(StringPtr("foo\0bar", 7)) == "foo%00bar");

  KJ_EXPECT(decodeUriComponent("foo%20bar") == "foo bar");
  KJ_EXPECT(decodeUriComponent("%ab%BA") == "\xab\xba");

  byte bytes[] = {12, 34, 56};
  KJ_EXPECT(decodeBinaryUriComponent(encodeUriComponent(bytes)).asPtr() == bytes);
}

KJ_TEST("C escape encoding/decoding") {
  KJ_EXPECT(encodeCEscape("fooo\a\b\f\n\r\t\v\'\"\\bar") ==
      "fooo\\a\\b\\f\\n\\r\\t\\v\\\'\\\"\\\\bar");
  KJ_EXPECT(encodeCEscape("foo\x01\x7fxxx") ==
      "foo\\001\\177xxx");

  expectUtf(decodeCEscape("fooo\\a\\b\\f\\n\\r\\t\\v\\\'\\\"\\\\bar"),
      "fooo\a\b\f\n\r\t\v\'\"\\bar");
  expectUtf(decodeCEscape("foo\\x01\\x7fxxx"), "foo\x01\x7fxxx");
  expectUtf(decodeCEscape("foo\\001\\177234"), "foo\001\177234");
  expectUtf(decodeCEscape("foo\\x1"), "foo\x1");
  expectUtf(decodeCEscape("foo\\1"), "foo\1");

  expectUtf(decodeCEscape("foo\\u1234bar"), u8"foo\u1234bar");
  expectUtf(decodeCEscape("foo\\U00045678bar"), u8"foo\U00045678bar");

  // Error cases.
  expectUtf(decodeCEscape("foo\\"), "foo", true);
  expectUtf(decodeCEscape("foo\\x123x"), u8"foo\x23x", true);
  expectUtf(decodeCEscape("foo\\u12"), u8"foo\u0012", true);
  expectUtf(decodeCEscape("foo\\u12xxx"), u8"foo\u0012xxx", true);
  expectUtf(decodeCEscape("foo\\U12"), u8"foo\u0012", true);
  expectUtf(decodeCEscape("foo\\U12xxxxxxxx"), u8"foo\u0012xxxxxxxx", true);
}

KJ_TEST("base64 encoding/decoding") {
  {
    auto encoded = encodeBase64(StringPtr("foo").asBytes(), false);
    KJ_EXPECT(encoded == "Zm9v", encoded, encoded.size());
    KJ_EXPECT(heapString(decodeBase64(encoded.asArray()).asChars()) == "foo");
  }

  {
    auto encoded = encodeBase64(StringPtr("corge").asBytes(), false);
    KJ_EXPECT(encoded == "Y29yZ2U=", encoded);
    KJ_EXPECT(heapString(decodeBase64(encoded.asArray()).asChars()) == "corge");
  }

  KJ_EXPECT(heapString(decodeBase64("Y29yZ2U").asChars()) == "corge");
  KJ_EXPECT(heapString(decodeBase64("Y\n29y Z@2U=\n").asChars()) == "corge");

  {
    auto encoded = encodeBase64(StringPtr("corge").asBytes(), true);
    KJ_EXPECT(encoded == "Y29yZ2U=\n", encoded);
  }

  StringPtr fullLine = "012345678901234567890123456789012345678901234567890123";
  {
    auto encoded = encodeBase64(fullLine.asBytes(), false);
    KJ_EXPECT(
        encoded == "MDEyMzQ1Njc4OTAxMjM0NTY3ODkwMTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0NTY3ODkwMTIz",
        encoded);
  }
  {
    auto encoded = encodeBase64(fullLine.asBytes(), true);
    KJ_EXPECT(
        encoded == "MDEyMzQ1Njc4OTAxMjM0NTY3ODkwMTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0NTY3ODkwMTIz\n",
        encoded);
  }

  String multiLine = str(fullLine, "456");
  {
    auto encoded = encodeBase64(multiLine.asBytes(), false);
    KJ_EXPECT(
        encoded == "MDEyMzQ1Njc4OTAxMjM0NTY3ODkwMTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0NTY3ODkwMTIzNDU2",
        encoded);
  }
  {
    auto encoded = encodeBase64(multiLine.asBytes(), true);
    KJ_EXPECT(
        encoded == "MDEyMzQ1Njc4OTAxMjM0NTY3ODkwMTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0NTY3ODkwMTIz\n"
                   "NDU2\n",
        encoded);
  }
}

}  // namespace
}  // namespace kj
