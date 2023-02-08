#ifndef THIRD_PARTY_ROBOTSTXT_REPORTING_ROBOTS_H_
#define THIRD_PARTY_ROBOTSTXT_REPORTING_ROBOTS_H_

#include <algorithm>
#include <string>
#include <vector>

#include "absl/log/check.h"
#include "robots.h"

namespace googlebot {
class RobotsParsingReporter : public googlebot::RobotsParseHandler {
 public:
  struct RobotsParsedLine {
    enum RobotsTagName {
      kUnknown = 0,
      kUserAgent = 1,
      kAllow = 2,
      kDisallow = 3,
      kSitemap = 4,
      kUnused = 5,
    };

    int line_num;
    RobotsTagName tag_name;
    bool is_typo;

    // For tests, probably.
    std::string toString() {
      std::ostringstream strout;
      strout << line_num << "," << tag_name << "," << is_typo;
      return strout.str();
    }
  };

  void HandleRobotsStart() override;
  void HandleRobotsEnd() override;
  void HandleUserAgent(int line_num, absl::string_view line_value) override;
  void HandleAllow(int line_num, absl::string_view line_value) override;
  void HandleDisallow(int line_num, absl::string_view line_value) override;
  void HandleSitemap(int line_num, absl::string_view line_value) override;
  void HandleUnknownAction(int line_num, absl::string_view action,
                           absl::string_view line_value) override;

  int last_line_seen() const { return last_line_seen_; }
  int valid_directives() const { return valid_directives_; }
  int unused_directives() const { return unused_directives_; }
  std::vector<RobotsParsedLine> parse_results() const {
    return robots_parse_results_;
  }

 private:
  void Digest(int line_num, RobotsParsedLine::RobotsTagName parsed_tag,
              bool is_typo);

  std::vector<RobotsParsedLine> robots_parse_results_;
  int last_line_seen_ = 0;
  int valid_directives_ = 0;
  int unused_directives_ = 0;
};
}  // namespace googlebot
#endif  // THIRD_PARTY_ROBOTSTXT_REPORTING_ROBOTS_H_
