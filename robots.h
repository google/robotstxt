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
// File: robots.h
// -----------------------------------------------------------------------------
//
// This file implements the standard defined by the Robots Exclusion Protocol
// (REP) internet draft (I-D).
//   https://tools.ietf.org/html/draft-koster-rep
//
// Google doesn't follow the standard strictly, because there are a lot of
// non-conforming robots.txt files out there, and we err on the side of
// disallowing when this seems intended.
//
// An more user-friendly description of how Google handles robots.txt can be
// found at:
//   https://developers.google.com/search/reference/robots_txt
//
// This library provides a low-level parser for robots.txt (ParseRobotsTxt()),
// and a matcher for URLs against a robots.txt (class RobotsMatcher).

#ifndef THIRD_PARTY_ROBOTSTXT_ROBOTS_H__
#define THIRD_PARTY_ROBOTSTXT_ROBOTS_H__

#include <string>
#include <vector>

#include "absl/strings/string_view.h"

namespace googlebot {
// Handler for directives found in robots.txt. These callbacks are called by
// ParseRobotsTxt() in the sequence they have been found in the file.
class RobotsParseHandler {
 public:
  RobotsParseHandler() {}
  virtual ~RobotsParseHandler() {}

  // Disallow copying and assignment.
  RobotsParseHandler(const RobotsParseHandler&) = delete;
  RobotsParseHandler& operator=(const RobotsParseHandler&) = delete;

  virtual void HandleRobotsStart() = 0;
  virtual void HandleRobotsEnd() = 0;

  virtual void HandleUserAgent(int line_num, absl::string_view value) = 0;
  virtual void HandleAllow(int line_num, absl::string_view value) = 0;
  virtual void HandleDisallow(int line_num, absl::string_view value) = 0;

  virtual void HandleSitemap(int line_num, absl::string_view value) = 0;

  // Any other unrecognized name/value pairs.
  virtual void HandleUnknownAction(int line_num, absl::string_view action,
                                   absl::string_view value) = 0;
};

// Parses body of a robots.txt and emits parse callbacks. This will accept
// typical typos found in robots.txt, such as 'disalow'.
//
// Note, this function will accept all kind of input but will skip
// everything that does not look like a robots directive.
void ParseRobotsTxt(absl::string_view robots_body,
                    RobotsParseHandler* parse_callback);

// RobotsMatcher - matches robots.txt against URLs.
//
// The Matcher uses a default match strategy for Allow/Disallow patterns which
// is the official way of Google crawler to match robots.txt. It is also
// possible to provide a custom match strategy.
//
// The entry point for the user is to call one of the *AllowedByRobots()
// methods that return directly if a URL is being allowed according to the
// robots.txt and the crawl agent.
// The RobotsMatcher can be re-used for URLs/robots.txt but is not thread-safe.
class RobotsMatchStrategy;
class RobotsMatcher : protected RobotsParseHandler {
 public:
  // Create a RobotsMatcher with the default matching strategy. The default
  // matching strategy is longest-match as opposed to the former internet draft
  // that provisioned first-match strategy. Analysis shows that longest-match,
  // while more restrictive for crawlers, is what webmasters assume when writing
  // directives. For example, in case of conflicting matches (both Allow and
  // Disallow), the longest match is the one the user wants. For example, in
  // case of a robots.txt file that has the following rules
  //   Allow: /
  //   Disallow: /cgi-bin
  // it's pretty obvious what the webmaster wants: they want to allow crawl of
  // every URI except /cgi-bin. However, according to the expired internet
  // standard, crawlers should be allowed to crawl everything with such a rule.
  RobotsMatcher();

  ~RobotsMatcher() override;

  // Disallow copying and assignment.
  RobotsMatcher(const RobotsMatcher&) = delete;
  RobotsMatcher& operator=(const RobotsMatcher&) = delete;

  // Verifies that the given user agent is valid to be matched against
  // robots.txt. Valid user agent strings only contain the characters
  // [a-zA-Z_-].
  static bool IsValidUserAgentToObey(absl::string_view user_agent);

  // Returns true iff 'url' is allowed to be fetched by any member of the
  // "user_agents" vector. 'url' must be %-encoded according to RFC3986.
  bool AllowedByRobots(absl::string_view robots_body,
                       const std::vector<std::string>* user_agents,
                       const std::string& url);

  // Do robots check for 'url' when there is only one user agent. 'url' must
  // be %-encoded according to RFC3986.
  bool OneAgentAllowedByRobots(absl::string_view robots_txt,
                               const std::string& user_agent,
                               const std::string& url);

  // Returns true if we are disallowed from crawling a matching URI.
  bool disallow() const;

  // Returns true if we are disallowed from crawling a matching URI. Ignores any
  // rules specified for the default user agent, and bases its results only on
  // the specified user agents.
  bool disallow_ignore_global() const;

  // Returns true iff, when AllowedByRobots() was called, the robots file
  // referred explicitly to one of the specified user agents.
  bool ever_seen_specific_agent() const;

  // Returns the line that matched or 0 if none matched.
  const int matching_line() const;

 protected:
  // Parse callbacks.
  // Protected because used in unittest. Never override RobotsMatcher, implement
  // googlebot::RobotsParseHandler instead.
  void HandleRobotsStart() override;
  void HandleRobotsEnd() override {}

  void HandleUserAgent(int line_num, absl::string_view value) override;
  void HandleAllow(int line_num, absl::string_view value) override;
  void HandleDisallow(int line_num, absl::string_view value) override;

  void HandleSitemap(int line_num, absl::string_view value) override;
  void HandleUnknownAction(int line_num, absl::string_view action,
                           absl::string_view value) override;

 protected:
  // Extract the matchable part of a user agent string, essentially stopping at
  // the first invalid character.
  // Example: 'Googlebot/2.1' becomes 'Googlebot'
  static absl::string_view ExtractUserAgent(absl::string_view user_agent);

  // Initialize next path and user-agents to check. Path must contain only the
  // path, params, and query (if any) of the url and must start with a '/'.
  void InitUserAgentsAndPath(const std::vector<std::string>* user_agents,
                             const char* path);

  // Returns true if any user-agent was seen.
  bool seen_any_agent() const {
    return seen_global_agent_ || seen_specific_agent_;
  }

  // Instead of just maintaining a Boolean indicating whether a given line has
  // matched, we maintain a count of the maximum number of characters matched by
  // that pattern.
  //
  // This structure stores the information associated with a match (e.g. when a
  // Disallow is matched) as priority of the match and line matching.
  //
  // The priority is initialized with a negative value to make sure that a match
  // of priority 0 is higher priority than no match at all.
  class Match {
   private:
    static const int kNoMatchPriority = -1;

   public:
    Match(int priority, int line) : priority_(priority), line_(line) {}
    Match() : priority_(kNoMatchPriority), line_(0) {}

    void Set(int priority, int line) {
      priority_ = priority;
      line_ = line;
    }

    void Clear() { Set(kNoMatchPriority, 0); }

    int line() const { return line_; }
    int priority() const { return priority_; }

    static const Match& HigherPriorityMatch(const Match& a, const Match& b) {
      if (a.priority() > b.priority()) {
        return a;
      } else {
        return b;
      }
    }

   private:
    int priority_;
    int line_;
  };

  // For each of the directives within user-agents, we keep global and specific
  // match scores.
  struct MatchHierarchy {
    Match global;            // Match for '*'
    Match specific;          // Match for queried agent.
    void Clear() {
      global.Clear();
      specific.Clear();
    }
  };
  MatchHierarchy allow_;       // Characters of 'url' matching Allow.
  MatchHierarchy disallow_;    // Characters of 'url' matching Disallow.

  bool seen_global_agent_;         // True if processing global agent rules.
  bool seen_specific_agent_;       // True if processing our specific agent.
  bool ever_seen_specific_agent_;  // True if we ever saw a block for our agent.
  bool seen_separator_;            // True if saw any key: value pair.

  // The path we want to pattern match. Not owned and only a valid pointer
  // during the lifetime of *AllowedByRobots calls.
  const char* path_;
  // The User-Agents we are interested in. Not owned and only a valid
  // pointer during the lifetime of *AllowedByRobots calls.
  const std::vector<std::string>* user_agents_;

  RobotsMatchStrategy* match_strategy_;
};

}  // namespace googlebot
#endif  // THIRD_PARTY_ROBOTSTXT_ROBOTS_H__
