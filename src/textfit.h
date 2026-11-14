#pragma once
// Shortening of labels that must fit a given width (pure logic, unit-tested).

#include <string>

// Removes characters from the end of `name` and appends "." until `fits(name)`
// is true or only `minChars` characters remain. UTF-8 sequences are never cut
// in the middle.
template <class Fits>
std::string shortenToFit(std::string name, size_t minChars, Fits fits) {
  auto charCount = [](const std::string& s) {
    size_t n = 0;
    for (unsigned char c : s) n += (c & 0xC0) != 0x80;
    return n;
  };
  bool shortened = false;
  while (!fits(name)) {
    std::string base = shortened ? name.substr(0, name.size() - 1) : name;   // drop "."
    if (charCount(base) <= minChars) break;
    size_t cut = base.size() - 1;
    while (cut > 0 && (static_cast<unsigned char>(base[cut]) & 0xC0) == 0x80) --cut;
    base.erase(cut);
    while (!base.empty() && base.back() == ' ') base.pop_back();
    name = base + ".";
    shortened = true;
  }
  return name;
}
