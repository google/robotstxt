#include "reporting_robots.h"

#include "gtest/gtest.h"

using ::googlebot::RobotsParsingReporter;

TEST(RobotsUnittest, LinesNumbersAreCountedCorrectly) {
  RobotsParsingReporter report;
  static const char kSimpleFile[] =
      "User-Agent: foo\n"         // 1
      "Allow: /some/path\n"       // 2
      "User-Agent: bar\n"         // 3
      "ansolutely random line\n"  // 4
      "#so comment, much wow\n"   // 5
      "\n"                        // 6
      "unicorns: /extinct\n"      // 7
      "noarchive: /some\n"        // 8
      "Disallow: /\n";            // 9
  googlebot::ParseRobotsTxt(kSimpleFile, &report);
  EXPECT_EQ(4, report.valid_directives());
  EXPECT_EQ(9, report.last_line_seen());

  EXPECT_EQ(report.parse_results()[0].toString(), "1,1,0");
  EXPECT_EQ(report.parse_results()[1].toString(), "2,2,0");
  EXPECT_EQ(report.parse_results()[2].toString(), "3,1,0");
  EXPECT_EQ(report.parse_results()[3].toString(), "7,0,0");
  EXPECT_EQ(report.parse_results()[4].toString(), "8,5,0");
  EXPECT_EQ(report.parse_results()[5].toString(), "9,3,0");

  static const char kDosFile[] =
      "User-Agent: foo\r\n"
      "Allow: /some/path\r\n"
      "User-Agent: bar\r\n"
      "\r\n"
      "\r\n"
      "Disallow: /\r\n";
  googlebot::ParseRobotsTxt(kDosFile, &report);
  EXPECT_EQ(4, report.valid_directives());
  EXPECT_EQ(6, report.last_line_seen());

  static const char kMacFile[] =
      "User-Agent: foo\r"
      "Allow: /some/path\r"
      "User-Agent: bar\r"
      "\r"
      "\r"
      "Disallow: /\r";
  googlebot::ParseRobotsTxt(kMacFile, &report);
  EXPECT_EQ(4, report.valid_directives());
  EXPECT_EQ(6, report.last_line_seen());
}
