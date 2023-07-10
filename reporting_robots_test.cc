#include "reporting_robots.h"

#include "gtest/gtest.h"
#include "absl/strings/str_split.h"

using ::googlebot::RobotsParsedLine;
using ::googlebot::RobotsParseHandler;
using ::googlebot::RobotsParsingReporter;

namespace {
// Allows debugging the contents of the LineMetadata struct.
std::string LineMetadataToString(const RobotsParseHandler::LineMetadata& line) {
  return absl::StrCat("{ is_empty: ", line.is_empty,
                      " has_directive: ", line.has_directive,
                      " has_comment: ", line.has_comment,
                      " is_comment: ", line.is_comment, " }");
}

std::string TagNameToString(RobotsParsedLine::RobotsTagName tag_name) {
  switch (tag_name) {
    case RobotsParsedLine::RobotsTagName::kUnknown:
      return "Unknown";
    case RobotsParsedLine::RobotsTagName::kUserAgent:
      return "UserAgent";
    case RobotsParsedLine::RobotsTagName::kAllow:
      return "Allow";
    case RobotsParsedLine::RobotsTagName::kDisallow:
      return "Disallow";
    case RobotsParsedLine::RobotsTagName::kSitemap:
      return "Sitemap";
    case RobotsParsedLine::RobotsTagName::kUnused:
      return "Unused";
  }
}

// Allows debugging the contents of the RobotsParsedLine struct.
std::string RobotsParsedLineToString(const RobotsParsedLine& line) {
  return absl::StrCat("{\n lin_num:", line.line_num,
                      "\n tag_name: ", TagNameToString(line.tag_name),
                      "\n is_typo: ", line.is_typo,
                      "\n metadata: ", LineMetadataToString(line.metadata),
                      "\n}");
}

void expectLineToParseTo(const std::vector<absl::string_view>& lines,
                         const std::vector<RobotsParsedLine>& parse_results,
                         const RobotsParsedLine& expected_result) {
  int line_num = expected_result.line_num;
  EXPECT_EQ(parse_results[line_num - 1], expected_result)
      << "For line " << line_num << ": '" << lines[line_num - 1] << "'";
}
}  // namespace

namespace googlebot {
// This allows us to get a debug content of the object in the test. Without
// this, it would say something like this when a test fails:
//  Expected equality of these values:
//   parse_results[line_num - 1]
//    Which is: 16-byte object <01-00 00-00 01-00 00-00 00-00 00-00 01-00 00-00>
//   expected_result
//    Which is: 16-byte object <01-00 00-00 01-00 00-00 00-00 00-00 00-25 00-00>
std::ostream& operator<<(std::ostream& o, const RobotsParsedLine& line) {
  o << RobotsParsedLineToString(line);
  return o;
}

// These 2 `operator==` are needed for `EXPECT_EQ` to work.
bool operator==(const RobotsParseHandler::LineMetadata& lhs,
                const RobotsParseHandler::LineMetadata& rhs) {
  return lhs.is_empty == rhs.is_empty &&
         lhs.has_directive == rhs.has_directive &&
         lhs.has_comment == rhs.has_comment && lhs.is_comment == rhs.is_comment;
}

bool operator==(const RobotsParsedLine& lhs, const RobotsParsedLine& rhs) {
  return lhs.line_num == rhs.line_num && lhs.tag_name == rhs.tag_name &&
         lhs.is_typo == rhs.is_typo && lhs.metadata == rhs.metadata;
}
}  // namespace googlebot

TEST(RobotsUnittest, LinesNumbersAreCountedCorrectly) {
  RobotsParsingReporter report;
  static const char kSimpleFile[] =
      "User-Agent: foo\n"         // 1
      "Allow: /some/path\n"       // 2
      "User-Agent: bar # no\n"    // 3
      "absolutely random line\n"  // 4
      "#so comment, much wow\n"   // 5
      "\n"                        // 6
      "unicorns: /extinct\n"      // 7
      "noarchive: /some\n"        // 8
      "Disallow: /\n"             // 9
      "Error #and comment\n";     // 10
                                  // 11 (from \n)
  googlebot::ParseRobotsTxt(kSimpleFile, &report);
  EXPECT_EQ(4, report.valid_directives());
  EXPECT_EQ(11, report.last_line_seen());
  EXPECT_EQ(report.parse_results().size(), report.last_line_seen());
  std::vector<absl::string_view> lines = absl::StrSplit(kSimpleFile, '\n');

  // For line "User-Agent: foo\n"         // 1
  expectLineToParseTo(
      lines, report.parse_results(),
      RobotsParsedLine{.line_num = 1,
                       .tag_name = RobotsParsedLine::RobotsTagName::kUserAgent,
                       .is_typo = false,
                       .metadata = RobotsParseHandler::LineMetadata{
                           .is_empty = false,
                           .has_comment = false,
                           .is_comment = false,
                           .has_directive = true,
                       }});
  // For line "Allow: /some/path\n"       // 2
  expectLineToParseTo(
      lines, report.parse_results(),
      RobotsParsedLine{.line_num = 2,
                       .tag_name = RobotsParsedLine::RobotsTagName::kAllow,
                       .is_typo = false,
                       .metadata = RobotsParseHandler::LineMetadata{
                           .is_empty = false,
                           .has_comment = false,
                           .is_comment = false,
                           .has_directive = true,
                       }});
  // For line "User-Agent: bar # no\n"    // 3
  expectLineToParseTo(
      lines, report.parse_results(),
      RobotsParsedLine{.line_num = 3,
                       .tag_name = RobotsParsedLine::RobotsTagName::kUserAgent,
                       .is_typo = false,
                       .metadata = RobotsParseHandler::LineMetadata{
                           .is_empty = false,
                           .has_comment = true,
                           .is_comment = false,
                           .has_directive = true,
                       }});
  // For line "absolutely random line\n"  // 4
  expectLineToParseTo(
      lines, report.parse_results(),
      RobotsParsedLine{.line_num = 4,
                       .tag_name = RobotsParsedLine::RobotsTagName::kUnknown,
                       .is_typo = false,
                       .metadata = RobotsParseHandler::LineMetadata{
                           .is_empty = false,
                           .has_comment = false,
                           .is_comment = false,
                           .has_directive = false,
                       }});
  // For line "#so comment, much wow\n"   // 5
  expectLineToParseTo(
      lines, report.parse_results(),
      RobotsParsedLine{.line_num = 5,
                       .tag_name = RobotsParsedLine::RobotsTagName::kUnknown,
                       .is_typo = false,
                       .metadata = RobotsParseHandler::LineMetadata{
                           .is_empty = false,
                           .has_comment = true,
                           .is_comment = true,
                           .has_directive = false,
                       }});
  // For line "\n"                        // 6
  expectLineToParseTo(
      lines, report.parse_results(),
      RobotsParsedLine{.line_num = 6,
                       .tag_name = RobotsParsedLine::RobotsTagName::kUnknown,
                       .is_typo = false,
                       .metadata = RobotsParseHandler::LineMetadata{
                           .is_empty = true,
                           .has_comment = false,
                           .is_comment = false,
                           .has_directive = false,
                       }});
  // For line "unicorns: /extinct\n"      // 7
  expectLineToParseTo(
      lines, report.parse_results(),
      RobotsParsedLine{.line_num = 7,
                       .tag_name = RobotsParsedLine::RobotsTagName::kUnknown,
                       .is_typo = false,
                       .metadata = RobotsParseHandler::LineMetadata{
                           .is_empty = false,
                           .has_comment = false,
                           .is_comment = false,
                           .has_directive = true,
                       }});
  // For line "noarchive: /some\n"        // 8
  expectLineToParseTo(
      lines, report.parse_results(),
      RobotsParsedLine{.line_num = 8,
                       .tag_name = RobotsParsedLine::RobotsTagName::kUnused,
                       .is_typo = false,
                       .metadata = RobotsParseHandler::LineMetadata{
                           .is_empty = false,
                           .has_comment = false,
                           .is_comment = false,
                           .has_directive = true,
                       }});
  // For line "Disallow: /\n"             // 9
  expectLineToParseTo(
      lines, report.parse_results(),
      RobotsParsedLine{.line_num = 9,
                       .tag_name = RobotsParsedLine::RobotsTagName::kDisallow,
                       .is_typo = false,
                       .metadata = RobotsParseHandler::LineMetadata{
                           .is_empty = false,
                           .has_comment = false,
                           .is_comment = false,
                           .has_directive = true,
                       }});
  // For line "Error #and comment\n";     // 10
  expectLineToParseTo(
      lines, report.parse_results(),
      RobotsParsedLine{.line_num = 10,
                       .tag_name = RobotsParsedLine::RobotsTagName::kUnknown,
                       .is_typo = false,
                       .metadata = RobotsParseHandler::LineMetadata{
                           .is_empty = false,
                           .has_comment = true,
                           .is_comment = false,
                           .has_directive = false,
                       }});
  // For line 11 (which is empty and comes from the last \n)
  expectLineToParseTo(
      lines, report.parse_results(),
      RobotsParsedLine{.line_num = 11,
                       .tag_name = RobotsParsedLine::RobotsTagName::kUnknown,
                       .is_typo = false,
                       .metadata = RobotsParseHandler::LineMetadata{
                           .is_empty = true,
                           .has_comment = false,
                           .is_comment = false,
                           .has_directive = false,
                       }});

  static const char kDosFile[] =
      "User-Agent: foo\r\n"
      "Allow: /some/path\r\n"
      "User-Agent: bar\r\n"
      "\r\n"
      "\r\n"
      "Disallow: /\r\n";
  googlebot::ParseRobotsTxt(kDosFile, &report);
  EXPECT_EQ(4, report.valid_directives());
  EXPECT_EQ(7, report.last_line_seen());

  static const char kMacFile[] =
      "User-Agent: foo\r"
      "Allow: /some/path\r"
      "User-Agent: bar\r"
      "\r"
      "\r"
      "Disallow: /\r";
  googlebot::ParseRobotsTxt(kMacFile, &report);
  EXPECT_EQ(4, report.valid_directives());
  EXPECT_EQ(7, report.last_line_seen());
}
