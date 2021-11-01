// Copyright 1999 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// -----------------------------------------------------------------------------
// File: robots.cc
// -----------------------------------------------------------------------------
//
// Implements expired internet draft
//   http://www.robotstxt.org/norobots-rfc.txt
// with Google-specific optimizations detailed at
//   https://developers.google.com/search/reference/robots_txt

#include "robots.h"

#include <stdlib.h>

#include <cstddef>
#include <vector>

#include "absl/base/macros.h"
#include "absl/container/fixed_array.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "absl/strings/string_view.h"

// Allow for typos such as DISALOW in robots.txt.
static bool kAllowFrequentTypos = true;

namespace googlebot {

// A RobotsMatchStrategy defines a strategy for matching individual lines in a
// robots.txt file. Each Match* method should return a match priority, which is
// interpreted as:
//
// match priority < 0:
//    No match.
//
// match priority == 0:
//    Match, but treat it as if matched an empty pattern.
//
// match priority > 0:
//    Match.
class RobotsMatchStrategy {
 public:
  virtual ~RobotsMatchStrategy() {}

  virtual int MatchAllow(absl::string_view path,
                         absl::string_view pattern) = 0;
  virtual int MatchDisallow(absl::string_view path,
                            absl::string_view pattern) = 0;

 protected:
  // Implements robots.txt pattern matching.
  static bool Matches(absl::string_view path, absl::string_view pattern);
};

// Returns true if URI path matches the specified pattern. Pattern is anchored
// at the beginning of path. '$' is special only at the end of pattern.
//
// Since 'path' and 'pattern' are both externally determined (by the webmaster),
// we make sure to have acceptable worst-case performance.
/* static */ bool RobotsMatchStrategy::Matches(
    absl::string_view path, absl::string_view pattern) {
  const size_t pathlen = path.length();
  absl::FixedArray<size_t> pos(pathlen + 1);
  int numpos;

  // The pos[] array holds a sorted list of indexes of 'path', with length
  // 'numpos'.  At the start and end of each iteration of the main loop below,
  // the pos[] array will hold a list of the prefixes of the 'path' which can
  // match the current prefix of 'pattern'. If this list is ever empty,
  // return false. If we reach the end of 'pattern' with at least one element
  // in pos[], return true.

  pos[0] = 0;
  numpos = 1;

  for (auto pat = pattern.begin(); pat != pattern.end(); ++pat) {
    if (*pat == '$' && pat + 1 == pattern.end()) {
      return (pos[numpos - 1] == pathlen);
    }
    if (*pat == '*') {
      numpos = pathlen - pos[0] + 1;
      for (int i = 1; i < numpos; i++) {
        pos[i] = pos[i-1] + 1;
      }
    } else {
      // Includes '$' when not at end of pattern.
      int newnumpos = 0;
      for (int i = 0; i < numpos; i++) {
        if (pos[i] < pathlen && path[pos[i]] == *pat) {
          pos[newnumpos++] = pos[i] + 1;
        }
      }
      numpos = newnumpos;
      if (numpos == 0) return false;
    }
  }

  return true;
}

static const char* kHexDigits = "0123456789ABCDEF";

// GetPathParamsQuery is not in anonymous namespace to allow testing.
//
// Extracts path (with params) and query part from URL. Removes scheme,
// authority, and fragment. Result always starts with "/".
// Returns "/" if the url doesn't have a path or is not valid.
std::string GetPathParamsQuery(const std::string& url) {
  std::string path;

  // Initial two slashes are ignored.
  size_t search_start = 0;
  if (url.size() >= 2 && url[0] == '/' && url[1] == '/') search_start = 2;

  size_t early_path = url.find_first_of("/?;", search_start);
  size_t protocol_end = url.find("://", search_start);
  if (early_path < protocol_end) {
    // If path, param or query starts before ://, :// doesn't indicate protocol.
    protocol_end = std::string::npos;
  }
  if (protocol_end == std::string::npos) {
    protocol_end = search_start;
  } else {
    protocol_end += 3;
  }

  size_t path_start = url.find_first_of("/?;", protocol_end);
  if (path_start != std::string::npos) {
    size_t hash_pos = url.find('#', search_start);
    if (hash_pos < path_start) return "/";
    size_t path_end = (hash_pos == std::string::npos) ? url.size() : hash_pos;
    if (url[path_start] != '/') {
      // Prepend a slash if the result would start e.g. with '?'.
      return "/" + url.substr(path_start, path_end - path_start);
    }
    return url.substr(path_start, path_end - path_start);
  }

  return "/";
}

// MaybeEscapePattern is not in anonymous namespace to allow testing.
//
// Canonicalize the allowed/disallowed paths. For example:
//     /SanJoséSellers ==> /Sanjos%C3%A9Sellers
//     %aa ==> %AA
// When the function returns, (*dst) either points to src, or is newly
// allocated.
// Returns true if dst was newly allocated.
bool MaybeEscapePattern(const char* src, char** dst) {
  int num_to_escape = 0;
  bool need_capitalize = false;

  // First, scan the buffer to see if changes are needed. Most don't.
  for (int i = 0; src[i] != 0; i++) {
    // (a) % escape sequence.
    if (src[i] == '%' &&
        absl::ascii_isxdigit(src[i+1]) && absl::ascii_isxdigit(src[i+2])) {
      if (absl::ascii_islower(src[i+1]) || absl::ascii_islower(src[i+2])) {
        need_capitalize = true;
      }
      i += 2;
    // (b) needs escaping.
    } else if (src[i] & 0x80) {
      num_to_escape++;
    }
    // (c) Already escaped and escape-characters normalized (eg. %2f -> %2F).
  }
  // Return if no changes needed.
  if (!num_to_escape && !need_capitalize) {
    (*dst) = const_cast<char*>(src);
    return false;
  }
  (*dst) = new char[num_to_escape * 2 + strlen(src) + 1];
  int j = 0;
  for (int i = 0; src[i] != 0; i++) {
    // (a) Normalize %-escaped sequence (eg. %2f -> %2F).
    if (src[i] == '%' &&
        absl::ascii_isxdigit(src[i+1]) && absl::ascii_isxdigit(src[i+2])) {
      (*dst)[j++] = src[i++];
      (*dst)[j++] = absl::ascii_toupper(src[i++]);
      (*dst)[j++] = absl::ascii_toupper(src[i]);
      // (b) %-escape octets whose highest bit is set. These are outside the
      // ASCII range.
    } else if (src[i] & 0x80) {
      (*dst)[j++] = '%';
      (*dst)[j++] = kHexDigits[(src[i] >> 4) & 0xf];
      (*dst)[j++] = kHexDigits[src[i] & 0xf];
    // (c) Normal character, no modification needed.
    } else {
      (*dst)[j++] = src[i];
    }
  }
  (*dst)[j] = 0;
  return true;
}

// Internal helper classes and functions.
namespace {

// A robots.txt has lines of key/value pairs. A ParsedRobotsKey represents
// a key. This class can parse a text-representation (including common typos)
// and represent them as an enumeration which allows for faster processing
// afterwards.
// For unparsable keys, the original string representation is kept.
class ParsedRobotsKey {
 public:
  enum KeyType {
    // Generic highlevel fields.
    USER_AGENT,
    SITEMAP,

    // Fields within a user-agent.
    ALLOW,
    DISALLOW,

    // Unrecognized field; kept as-is. High number so that additions to the
    // enumeration above does not change the serialization.
    UNKNOWN = 128
  };

  ParsedRobotsKey() : type_(UNKNOWN) {}

  // Disallow copying and assignment.
  ParsedRobotsKey(const ParsedRobotsKey&) = delete;
  ParsedRobotsKey& operator=(const ParsedRobotsKey&) = delete;

  // Parse given key text. Does not copy the text, so the text_key must stay
  // valid for the object's life-time or the next Parse() call.
  void Parse(absl::string_view key);

  // Returns the type of key.
  KeyType type() const { return type_; }

  // If this is an unknown key, get the text.
  absl::string_view GetUnknownText() const;

 private:
  static bool KeyIsUserAgent(absl::string_view key);
  static bool KeyIsAllow(absl::string_view key);
  static bool KeyIsDisallow(absl::string_view key);
  static bool KeyIsSitemap(absl::string_view key);

  KeyType type_;
  absl::string_view key_text_;
};

void EmitKeyValueToHandler(int line, const ParsedRobotsKey& key,
                           absl::string_view value,
                           RobotsParseHandler* handler) {
  typedef ParsedRobotsKey Key;
  switch (key.type()) {
    case Key::USER_AGENT:     handler->HandleUserAgent(line, value); break;
    case Key::ALLOW:          handler->HandleAllow(line, value); break;
    case Key::DISALLOW:       handler->HandleDisallow(line, value); break;
    case Key::SITEMAP:        handler->HandleSitemap(line, value); break;
    case Key::UNKNOWN:
      handler->HandleUnknownAction(line, key.GetUnknownText(), value);
      break;
      // No default case Key:: to have the compiler warn about new values.
  }
}

class RobotsTxtParser {
 public:
  typedef ParsedRobotsKey Key;

  RobotsTxtParser(absl::string_view robots_body,
                  RobotsParseHandler* handler)
      : robots_body_(robots_body), handler_(handler) {
  }

  void Parse();

 private:
  static bool GetKeyAndValueFrom(char ** key, char **value, char *line);
  static void StripWhitespaceSlowly(char ** s);

  void ParseAndEmitLine(int current_line, char* line);
  bool NeedEscapeValueForKey(const Key& key);

  absl::string_view robots_body_;
  RobotsParseHandler* const handler_;
};

bool RobotsTxtParser::NeedEscapeValueForKey(const Key& key) {
  switch (key.type()) {
    case RobotsTxtParser::Key::USER_AGENT:
    case RobotsTxtParser::Key::SITEMAP:
      return false;
    default:
      return true;
  }
}

// Removes leading and trailing whitespace from null-terminated string s.
/* static */ void RobotsTxtParser::StripWhitespaceSlowly(char ** s) {
  absl::string_view stripped = absl::StripAsciiWhitespace(*s);
  *s = const_cast<char*>(stripped.data());
  (*s)[stripped.size()] = '\0';
}

bool RobotsTxtParser::GetKeyAndValueFrom(char ** key, char ** value,
                                         char * line) {
  // Remove comments from the current robots.txt line.
  char* const comment = strchr(line, '#');
  if (nullptr != comment) {
          *comment = '\0';
  }
  StripWhitespaceSlowly(&line);

  // Rules must match the following pattern:
  //   <key>[ \t]*:[ \t]*<value>
  char* sep = strchr(line, ':');
  if (nullptr == sep) {
    // Google-specific optimization: some people forget the colon, so we need to
    // accept whitespace in its stead.
    static const char * const kWhite = " \t";
    sep = strpbrk(line, kWhite);
    if (nullptr != sep) {
      const char* const val = sep + strspn(sep, kWhite);
      assert(*val);  // since we dropped trailing whitespace above.
      if (nullptr != strpbrk(val, kWhite)) {
        // We only accept whitespace as a separator if there are exactly two
        // sequences of non-whitespace characters.  If we get here, there were
        // more than 2 such sequences since we stripped trailing whitespace
        // above.
        return false;
      }
    }
  }
  if (nullptr == sep) {
    return false;                     // Couldn't find a separator.
  }

  *key = line;                        // Key starts at beginning of line.
  *sep = '\0';                        // And stops at the separator.
  StripWhitespaceSlowly(key);         // Get rid of any trailing whitespace.

  if (strlen(*key) > 0) {
    *value = 1 + sep;                 // Value starts after the separator.
    StripWhitespaceSlowly(value);     // Get rid of any leading whitespace.
    return true;
  }
  return false;
}

void RobotsTxtParser::ParseAndEmitLine(int current_line, char* line) {
  char* string_key;
  char* value;
  if (!GetKeyAndValueFrom(&string_key, &value, line)) {
    return;
  }

  Key key;
  key.Parse(string_key);
  if (NeedEscapeValueForKey(key)) {
    char* escaped_value = nullptr;
    const bool is_escaped = MaybeEscapePattern(value, &escaped_value);
    EmitKeyValueToHandler(current_line, key, escaped_value, handler_);
    if (is_escaped) delete[] escaped_value;
  } else {
    EmitKeyValueToHandler(current_line, key, value, handler_);
  }
}

void RobotsTxtParser::Parse() {
  // UTF-8 byte order marks.
  static const unsigned char utf_bom[3] = {0xEF, 0xBB, 0xBF};

  // Certain browsers limit the URL length to 2083 bytes. In a robots.txt, it's
  // fairly safe to assume any valid line isn't going to be more than many times
  // that max url length of 2KB. We want some padding for
  // UTF-8 encoding/nulls/etc. but a much smaller bound would be okay as well.
  // If so, we can ignore the chars on a line past that.
  const int kMaxLineLen = 2083 * 8;
  // Allocate a buffer used to process the current line.
  char* const line_buffer = new char[kMaxLineLen];
  // last_line_pos is the last writeable pos within the line array
  // (only a final '\0' may go here).
  const char* const line_buffer_end = line_buffer + kMaxLineLen - 1;
  char* line_pos = line_buffer;
  int line_num = 0;
  size_t bom_pos = 0;
  bool last_was_carriage_return = false;
  handler_->HandleRobotsStart();

      {
        for (const unsigned char ch : robots_body_) {
      ABSL_ASSERT(line_pos <= line_buffer_end);
      // Google-specific optimization: UTF-8 byte order marks should never
      // appear in a robots.txt file, but they do nevertheless. Skipping
      // possible BOM-prefix in the first bytes of the input.
      if (bom_pos < sizeof(utf_bom) && ch == utf_bom[bom_pos++]) {
        continue;
      }
      bom_pos = sizeof(utf_bom);
      if (ch != 0x0A && ch != 0x0D) {  // Non-line-ending char case.
        // Put in next spot on current line, as long as there's room.
        if (line_pos < line_buffer_end) {
          *(line_pos++) = ch;
        }
      } else {                         // Line-ending character char case.
        *line_pos = '\0';
        // Only emit an empty line if this was not due to the second character
        // of the DOS line-ending \r\n .
        const bool is_CRLF_continuation =
            (line_pos == line_buffer) && last_was_carriage_return && ch == 0x0A;
        if (!is_CRLF_continuation) {
          ParseAndEmitLine(++line_num, line_buffer);
        }
        line_pos = line_buffer;
        last_was_carriage_return = (ch == 0x0D);
      }
    }
  }
  *line_pos = '\0';
  ParseAndEmitLine(++line_num, line_buffer);
  handler_->HandleRobotsEnd();
  delete [] line_buffer;
}

// Implements the default robots.txt matching strategy. The maximum number of
// characters matched by a pattern is returned as its match priority.
class LongestMatchRobotsMatchStrategy : public RobotsMatchStrategy {
 public:
  LongestMatchRobotsMatchStrategy() { }

  // Disallow copying and assignment.
  LongestMatchRobotsMatchStrategy(const LongestMatchRobotsMatchStrategy&) =
      delete;
  LongestMatchRobotsMatchStrategy& operator=(
      const LongestMatchRobotsMatchStrategy&) = delete;

  int MatchAllow(absl::string_view path, absl::string_view pattern) override;
  int MatchDisallow(absl::string_view path, absl::string_view pattern) override;
};
}  // end anonymous namespace

void ParseRobotsTxt(absl::string_view robots_body,
                    RobotsParseHandler* parse_callback) {
  RobotsTxtParser parser(robots_body, parse_callback);
  parser.Parse();
}

RobotsMatcher::RobotsMatcher()
    : seen_global_agent_(false),
      seen_specific_agent_(false),
      ever_seen_specific_agent_(false),
      seen_separator_(false),
      path_(nullptr),
      user_agents_(nullptr) {
  match_strategy_ = new LongestMatchRobotsMatchStrategy();
}

RobotsMatcher::~RobotsMatcher() {
  delete match_strategy_;
}

bool RobotsMatcher::ever_seen_specific_agent() const {
  return ever_seen_specific_agent_;
}

void RobotsMatcher::InitUserAgentsAndPath(
    const std::vector<std::string>* user_agents, const char* path) {
  // The RobotsParser object doesn't own path_ or user_agents_, so overwriting
  // these pointers doesn't cause a memory leak.
  path_ = path;
  ABSL_ASSERT('/' == *path_);
  user_agents_ = user_agents;
}

bool RobotsMatcher::AllowedByRobots(absl::string_view robots_body,
                                    const std::vector<std::string>* user_agents,
                                    const std::string& url) {
  // The url is not normalized (escaped, percent encoded) here because the user
  // is asked to provide it in escaped form already.
  std::string path = GetPathParamsQuery(url);
  InitUserAgentsAndPath(user_agents, path.c_str());
  ParseRobotsTxt(robots_body, this);
  return !disallow();
}

bool RobotsMatcher::OneAgentAllowedByRobots(absl::string_view robots_txt,
                                            const std::string& user_agent,
                                            const std::string& url) {
  std::vector<std::string> v;
  v.push_back(user_agent);
  return AllowedByRobots(robots_txt, &v, url);
}

bool RobotsMatcher::disallow() const {
  if (allow_.specific.priority() > 0 || disallow_.specific.priority() > 0) {
    return (disallow_.specific.priority() > allow_.specific.priority());
  }

  if (ever_seen_specific_agent_) {
    // Matching group for user-agent but either without disallow or empty one,
    // i.e. priority == 0.
    return false;
  }

  if (disallow_.global.priority() > 0 || allow_.global.priority() > 0) {
    return disallow_.global.priority() > allow_.global.priority();
  }
  return false;
}

bool RobotsMatcher::disallow_ignore_global() const {
  if (allow_.specific.priority() > 0 || disallow_.specific.priority() > 0) {
    return disallow_.specific.priority() > allow_.specific.priority();
  }
  return false;
}

const int RobotsMatcher::matching_line() const {
  if (ever_seen_specific_agent_) {
    return Match::HigherPriorityMatch(disallow_.specific, allow_.specific)
        .line();
  }
  return Match::HigherPriorityMatch(disallow_.global, allow_.global).line();
}

void RobotsMatcher::HandleRobotsStart() {
  // This is a new robots.txt file, so we need to reset all the instance member
  // variables. We do it in the same order the instance member variables are
  // declared, so it's easier to keep track of which ones we have (or maybe
  // haven't!) done.
  allow_.Clear();
  disallow_.Clear();

  seen_global_agent_ = false;
  seen_specific_agent_ = false;
  ever_seen_specific_agent_ = false;
  seen_separator_ = false;
}

/*static*/ absl::string_view RobotsMatcher::ExtractUserAgent(
    absl::string_view user_agent) {
  // Allowed characters in user-agent are [a-zA-Z_-].
  const char* end = user_agent.data();
  while (absl::ascii_isalpha(*end) || *end == '-' || *end == '_') {
    ++end;
  }
  return user_agent.substr(0, end - user_agent.data());
}

/*static*/ bool RobotsMatcher::IsValidUserAgentToObey(
    absl::string_view user_agent) {
  return user_agent.length() > 0 && ExtractUserAgent(user_agent) == user_agent;
}

void RobotsMatcher::HandleUserAgent(int line_num,
                                    absl::string_view user_agent) {
  if (seen_separator_) {
    seen_specific_agent_ = seen_global_agent_ = seen_separator_ = false;
  }

  // Google-specific optimization: a '*' followed by space and more characters
  // in a user-agent record is still regarded a global rule.
  if (user_agent.length() >= 1 && user_agent[0] == '*' &&
      (user_agent.length() == 1 || isspace(user_agent[1]))) {
    seen_global_agent_ = true;
  } else {
    user_agent = ExtractUserAgent(user_agent);
    for (const auto& agent : *user_agents_) {
      if (absl::EqualsIgnoreCase(user_agent, agent)) {
        ever_seen_specific_agent_ = seen_specific_agent_ = true;
        break;
      }
    }
  }
}

void RobotsMatcher::HandleAllow(int line_num, absl::string_view value) {
  if (!seen_any_agent()) return;
  seen_separator_ = true;
  const int priority = match_strategy_->MatchAllow(path_, value);
  if (priority >= 0) {
    if (seen_specific_agent_) {
      if (allow_.specific.priority() < priority) {
        allow_.specific.Set(priority, line_num);
      }
    } else {
      assert(seen_global_agent_);
      if (allow_.global.priority() < priority) {
        allow_.global.Set(priority, line_num);
      }
    }
  } else {
    // Google-specific optimization: 'index.htm' and 'index.html' are normalized
    // to '/'.
    const size_t slash_pos = value.find_last_of('/');

    if (slash_pos != absl::string_view::npos &&
        absl::StartsWith(absl::ClippedSubstr(value, slash_pos),
                            "/index.htm")) {
      const int len = slash_pos + 1;
      absl::FixedArray<char> newpattern(len + 1);
      strncpy(newpattern.data(), value.data(), len);
      newpattern[len] = '$';
      HandleAllow(line_num,
                  absl::string_view(newpattern.data(), newpattern.size()));
    }
  }
}

void RobotsMatcher::HandleDisallow(int line_num, absl::string_view value) {
  if (!seen_any_agent()) return;
  seen_separator_ = true;
  const int priority = match_strategy_->MatchDisallow(path_, value);
  if (priority >= 0) {
    if (seen_specific_agent_) {
      if (disallow_.specific.priority() < priority) {
        disallow_.specific.Set(priority, line_num);
      }
    } else {
      assert(seen_global_agent_);
      if (disallow_.global.priority() < priority) {
        disallow_.global.Set(priority, line_num);
      }
    }
  }
}

int LongestMatchRobotsMatchStrategy::MatchAllow(absl::string_view path,
                                                absl::string_view pattern) {
  return Matches(path, pattern) ? pattern.length() : -1;
}

int LongestMatchRobotsMatchStrategy::MatchDisallow(absl::string_view path,
                                                   absl::string_view pattern) {
  return Matches(path, pattern) ? pattern.length() : -1;
}

void RobotsMatcher::HandleSitemap(int line_num, absl::string_view value) {}

void RobotsMatcher::HandleUnknownAction(int line_num, absl::string_view action,
                                        absl::string_view value) {}

void ParsedRobotsKey::Parse(absl::string_view key) {
  key_text_ = absl::string_view();
  if (KeyIsUserAgent(key)) {
    type_ = USER_AGENT;
  } else if (KeyIsAllow(key)) {
    type_ = ALLOW;
  } else if (KeyIsDisallow(key)) {
    type_ = DISALLOW;
  } else if (KeyIsSitemap(key)) {
    type_ = SITEMAP;
  } else {
    type_ = UNKNOWN;
    key_text_ = key;
  }
}

absl::string_view ParsedRobotsKey::GetUnknownText() const {
  ABSL_ASSERT(type_ == UNKNOWN && !key_text_.empty());
  return key_text_;
}

bool ParsedRobotsKey::KeyIsUserAgent(absl::string_view key) {
  return (
      absl::StartsWithIgnoreCase(key, "user-agent") ||
      (kAllowFrequentTypos && (absl::StartsWithIgnoreCase(key, "useragent") ||
                               absl::StartsWithIgnoreCase(key, "user agent"))));
}

bool ParsedRobotsKey::KeyIsAllow(absl::string_view key) {
  return absl::StartsWithIgnoreCase(key, "allow");
}

bool ParsedRobotsKey::KeyIsDisallow(absl::string_view key) {
  return (
      absl::StartsWithIgnoreCase(key, "disallow") ||
      (kAllowFrequentTypos && ((absl::StartsWithIgnoreCase(key, "dissallow")) ||
                               (absl::StartsWithIgnoreCase(key, "dissalow")) ||
                               (absl::StartsWithIgnoreCase(key, "disalow")) ||
                               (absl::StartsWithIgnoreCase(key, "diasllow")) ||
                               (absl::StartsWithIgnoreCase(key, "disallaw")))));
}

bool ParsedRobotsKey::KeyIsSitemap(absl::string_view key) {
  return ((absl::StartsWithIgnoreCase(key, "sitemap")) ||
          (absl::StartsWithIgnoreCase(key, "site-map")));
}

}  // namespace googlebot
