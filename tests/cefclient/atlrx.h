#pragma once

#include <Windows.h>
#include <regex>
#include <string>
#include <vector>

enum REParseError {
  REPARSE_ERROR_OK = 0,
  REPARSE_ERROR_ILLEGAL = 1,
};

template <class CharT = wchar_t>
class CAtlREMatchContext {
 public:
  void SetMatches(const CharT* base,
                  const std::vector<std::pair<size_t, size_t>>& ranges) {
    base_ = base;
    ranges_ = ranges;
  }

  void GetMatch(int index, const CharT** start, const CharT** end) const {
    if (!start || !end) {
      return;
    }
    *start = nullptr;
    *end = nullptr;
    if (index < 0 || static_cast<size_t>(index) >= ranges_.size() || !base_) {
      return;
    }
    const auto& range = ranges_[index];
    if (range.first == static_cast<size_t>(-1) ||
        range.second == static_cast<size_t>(-1)) {
      return;
    }
    *start = base_ + range.first;
    *end = base_ + range.second;
  }

 private:
  const CharT* base_ = nullptr;
  std::vector<std::pair<size_t, size_t>> ranges_;
};

template <class CharT = wchar_t>
class CAtlRegExp {
 public:
  REParseError Parse(const CharT* pattern, bool case_sensitive) {
    if (!pattern) {
      return REPARSE_ERROR_ILLEGAL;
    }

    std::basic_string<CharT> converted;
    converted.reserve(std::char_traits<CharT>::length(pattern) * 2);

    bool in_capture = false;
    for (const CharT* p = pattern; *p; ++p) {
      const CharT ch = *p;
      if (ch == L'{') {
        if (in_capture) {
          return REPARSE_ERROR_ILLEGAL;
        }
        converted += L'(';
        in_capture = true;
      } else if (ch == L'}') {
        if (!in_capture) {
          return REPARSE_ERROR_ILLEGAL;
        }
        converted += L')';
        in_capture = false;
      } else {
        converted += ch;
      }
    }

    if (in_capture) {
      return REPARSE_ERROR_ILLEGAL;
    }

    try {
      auto flags = std::regex_constants::ECMAScript;
      if (!case_sensitive) {
        flags |= std::regex_constants::icase;
      }
      regex_ = std::basic_regex<CharT>(converted, flags);
      parsed_ = true;
      return REPARSE_ERROR_OK;
    } catch (const std::regex_error&) {
      parsed_ = false;
      return REPARSE_ERROR_ILLEGAL;
    }
  }

  BOOL Match(const CharT* input,
             CAtlREMatchContext<CharT>* context,
             const CharT** next) const {
    if (!parsed_ || !input || !context || !next) {
      return FALSE;
    }

    std::match_results<const CharT*> matches;
    if (!std::regex_search(input, matches, regex_)) {
      return FALSE;
    }

    std::vector<std::pair<size_t, size_t>> ranges;
    ranges.reserve(matches.size() > 1 ? matches.size() - 1 : 0);
    for (size_t i = 1; i < matches.size(); ++i) {
      if (!matches[i].matched) {
        ranges.push_back({static_cast<size_t>(-1), static_cast<size_t>(-1)});
        continue;
      }
      const size_t start = static_cast<size_t>(matches[i].first - input);
      const size_t end = static_cast<size_t>(matches[i].second - input);
      ranges.push_back({start, end});
    }

    context->SetMatches(input, ranges);
    *next = matches[0].second;
    return TRUE;
  }

 private:
  std::basic_regex<CharT> regex_;
  bool parsed_ = false;
};
