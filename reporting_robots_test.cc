#include "reporting_robots.h"

#include <ostream>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "robots.h"

using ::googlebot::RobotsParsedLine;
using ::googlebot::RobotsParseHandler;
using ::googlebot::RobotsParsingReporter;

namespace {
// Allows debugging the contents of the LineMetadata struct.
std::string LineMetadataToString(const RobotsParseHandler::LineMetadata& line) {
  // clang-format off
  return absl::StrCat(
      "{ is_empty: ", line.is_empty,
      " has_directive: ", line.has_directive,
      " has_comment: ", line.has_comment,
      " is_comment: ", line.is_comment,
      " is_acceptable_typo: ", line.is_acceptable_typo,
      " is_line_too_long: ", line.is_line_too_long,
      " is_missing_colon_separator: ", line.is_missing_colon_separator, " }");
  // clang-format on
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
         lhs.has_comment == rhs.has_comment &&
         lhs.is_comment == rhs.is_comment &&
         lhs.is_acceptable_typo == rhs.is_acceptable_typo &&
         lhs.is_line_too_long == rhs.is_line_too_long &&
         lhs.is_missing_colon_separator == rhs.is_missing_colon_separator;
}

bool operator==(const RobotsParsedLine& lhs, const RobotsParsedLine& rhs) {
  return lhs.line_num == rhs.line_num && lhs.tag_name == rhs.tag_name &&
         lhs.is_typo == rhs.is_typo && lhs.metadata == rhs.metadata;
}
}  // namespace googlebot

TEST(RobotsUnittest, LinesNumbersAreCountedCorrectly) {
  RobotsParsingReporter report;
  static const char kSimpleFile[] =
      "User-Agent: foo\n"                     // 1
      "Allow: /some/path\n"                   // 2
      "User-Agent bar # no\n"                 // 3
      "absolutely random line\n"              // 4
      "#so comment, much wow\n"               // 5
      "\n"                                    // 6
      "unicorns: /extinct\n"                  // 7
      "noarchive: /some\n"                    // 8
      "Disallow: /\n"                         // 9
      "Error #and comment\n"                  // 10
      "useragent: baz\n"                      // 11
      "disallaw: /some\n"                     // 12
      "site-map: https://e/s.xml #comment\n"  // 13
      "sitemap: https://e/t.xml\n"            // 14
      "Noarchive: /someCapital\n";            // 15
                                              // 16 (from \n)
  googlebot::ParseRobotsTxt(kSimpleFile, &report);
  EXPECT_EQ(8, report.valid_directives());
  EXPECT_EQ(16, report.last_line_seen());
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
                           .is_missing_colon_separator = false,
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
  // For line "User-Agent bar # no\n"    // 3
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
                           .is_missing_colon_separator = true,
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
                           .is_missing_colon_separator = false,
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
                           .is_missing_colon_separator = false,
                       }});
  // For line "useragent: baz\n";         // 11
  expectLineToParseTo(
      lines, report.parse_results(),
      RobotsParsedLine{.line_num = 11,
                       .tag_name = RobotsParsedLine::RobotsTagName::kUserAgent,
                       .is_typo = true,
                       .metadata = RobotsParseHandler::LineMetadata{
                           .is_empty = false,
                           .has_comment = false,
                           .is_comment = false,
                           .has_directive = true,
                           .is_acceptable_typo = true,
                       }});
  // For line "disallaw: /some\n"         // 12
  expectLineToParseTo(
      lines, report.parse_results(),
      RobotsParsedLine{.line_num = 12,
                       .tag_name = RobotsParsedLine::RobotsTagName::kDisallow,
                       .is_typo = true,
                       .metadata = RobotsParseHandler::LineMetadata{
                           .is_empty = false,
                           .has_comment = false,
                           .is_comment = false,
                           .has_directive = true,
                           .is_acceptable_typo = true,
                       }});
  // For line "site-map: https://e/s.xml #comment\n"  // 13;
  expectLineToParseTo(
      lines, report.parse_results(),
      RobotsParsedLine{.line_num = 13,
                       .tag_name = RobotsParsedLine::RobotsTagName::kSitemap,
                       .is_typo = true,
                       .metadata = RobotsParseHandler::LineMetadata{
                           .is_empty = false,
                           .has_comment = true,
                           .is_comment = false,
                           .has_directive = true,
                           .is_acceptable_typo = true,
                       }});
  // For line "sitemap: https://e/t.xml\n"  // 14;
  expectLineToParseTo(
      lines, report.parse_results(),
      RobotsParsedLine{.line_num = 14,
                       .tag_name = RobotsParsedLine::RobotsTagName::kSitemap,
                       .is_typo = false,
                       .metadata = RobotsParseHandler::LineMetadata{
                           .is_empty = false,
                           .has_comment = false,
                           .is_comment = false,
                           .has_directive = true,
                           .is_acceptable_typo = false,
                       }});
  // For line "Noarchive: /someCapital\n"        // 15
  expectLineToParseTo(
      lines, report.parse_results(),
      RobotsParsedLine{.line_num = 15,
                       .tag_name = RobotsParsedLine::RobotsTagName::kUnused,
                       .is_typo = false,
                       .metadata = RobotsParseHandler::LineMetadata{
                           .is_empty = false,
                           .has_comment = false,
                           .is_comment = false,
                           .has_directive = true,
                       }});
  // For line 16 (which is empty and comes from the last \n)
  expectLineToParseTo(
      lines, report.parse_results(),
      RobotsParsedLine{.line_num = 16,
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

TEST(RobotsUnittest, LinesTooLongReportedCorrectly) {
  RobotsParsingReporter report;
  const int kMaxLineLen = 2084 * 8;
  std::string allow = "allow: /\n";
  std::string disallow = "disallow: ";
  std::string robotstxt = "user-agent: foo\n";
  std::string longline = "/x/";
  while (longline.size() < kMaxLineLen) {
    absl::StrAppend(&longline, "a");
  }
  absl::StrAppend(&robotstxt, disallow, longline, "\n", allow);

  googlebot::ParseRobotsTxt(robotstxt, &report);
  EXPECT_EQ(3, report.valid_directives());
  EXPECT_EQ(4, report.last_line_seen());
  EXPECT_EQ(report.parse_results().size(), report.last_line_seen());
  std::vector<absl::string_view> lines = absl::StrSplit(robotstxt, '\n');

  // For line "user-agent: foo\n"       // 1
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
                           .is_line_too_long = false,
                       }});
  // For line "disallow: /x/a[...]a\n"  // 2
  expectLineToParseTo(
      lines, report.parse_results(),
      RobotsParsedLine{.line_num = 2,
                       .tag_name = RobotsParsedLine::RobotsTagName::kDisallow,
                       .is_typo = false,
                       .metadata = RobotsParseHandler::LineMetadata{
                           .is_empty = false,
                           .has_comment = false,
                           .is_comment = false,
                           .has_directive = true,
                           .is_line_too_long = true,
                       }});
  // For line "allow: /\n"              // 3
  expectLineToParseTo(
      lines, report.parse_results(),
      RobotsParsedLine{.line_num = 3,
                       .tag_name = RobotsParsedLine::RobotsTagName::kAllow,
                       .is_typo = false,
                       .metadata = RobotsParseHandler::LineMetadata{
                           .is_empty = false,
                           .has_comment = false,
                           .is_comment = false,
                           .has_directive = true,
                           .is_line_too_long = false,
                       }});
}
