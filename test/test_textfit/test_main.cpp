// Unit tests for label shortening (run on the PC): pio test -e native
#include <unity.h>

#include <string>

#include "textfit.h"

// Width model: one unit per character (UTF-8 aware).
static size_t chars(const std::string& s) {
  size_t n = 0;
  for (unsigned char c : s) n += (c & 0xC0) != 0x80;
  return n;
}

void test_short_name_is_unchanged(void) {
  auto r = shortenToFit("Tokyo", 3, [](const std::string& s) { return chars(s) <= 10; });
  TEST_ASSERT_EQUAL_STRING("Tokyo", r.c_str());
}

void test_long_name_gets_dot(void) {
  auto r = shortenToFit("McMurdo Station", 3, [](const std::string& s) { return chars(s) <= 11; });
  TEST_ASSERT_EQUAL_STRING("McMurdo St.", r.c_str());
  TEST_ASSERT_TRUE(chars(r) <= 11);
}

void test_trailing_space_is_removed_before_dot(void) {
  auto r = shortenToFit("Cape Town", 3, [](const std::string& s) { return chars(s) <= 6; });
  TEST_ASSERT_EQUAL_STRING("Cape.", r.c_str());
}

void test_minimum_length_stops_shortening(void) {
  auto r = shortenToFit("Longyearbyen", 3, [](const std::string&) { return false; });
  TEST_ASSERT_EQUAL_STRING("Lon.", r.c_str());   // never fits: stop at 3 chars + "."
}

void test_utf8_is_not_cut_in_the_middle(void) {
  auto r = shortenToFit("東京都千代田区", 2, [](const std::string& s) { return chars(s) <= 4; });
  TEST_ASSERT_EQUAL_STRING("東京都.", r.c_str());
}

void test_empty_name(void) {
  auto r = shortenToFit("", 3, [](const std::string&) { return false; });
  TEST_ASSERT_EQUAL_STRING("", r.c_str());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_short_name_is_unchanged);
  RUN_TEST(test_long_name_gets_dot);
  RUN_TEST(test_trailing_space_is_removed_before_dot);
  RUN_TEST(test_minimum_length_stops_shortening);
  RUN_TEST(test_utf8_is_not_cut_in_the_middle);
  RUN_TEST(test_empty_name);
  return UNITY_END();
}
