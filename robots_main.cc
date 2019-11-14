// Copyright 2019 Google LLC
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
// File: robots_main.cc
// -----------------------------------------------------------------------------
//
// Simple binary to assess whether a URL is accessible to a user-agent according
// to records found in a local robots.txt file, based on Google's robots.txt
// parsing and matching algorithms.
// Usage (single user-agent):
//     robots_main <local_path_to_robotstxt> <user_agent> <url>
// Arguments:
// local_path_to_robotstxt: local path to a file containing robots.txt records.
//   For example: /home/users/username/robots.txt
// user_agent: a token to be matched against records in the robots.txt.
//   For example: Googlebot
// url: a url to be matched against records in the robots.txt. The URL must be
// %-encoded according to RFC3986.
//   For example: https://example.com/accessible/url.html
// Returns: Prints a sentence with verdict about whether 'user_agent' is allowed
// to access 'url' based on records in 'local_path_to_robotstxt'.
//
// Usage (pair of user-agents):
//     robots_main <local_path_to_robotstxt> <comma_sep_user_agent_pair> <url>
// Arguments:
// local_path_to_robotstxt: local path to a file containing robots.txt records.
//   For example: /home/users/username/robots.txt
// comma_sep_user_agent_pair: pair of user-agents (most specific first)
//   For example: googlebot-image,googlebot
// url: a url to be matched against records in the robots.txt. The URL must be
// %-encoded according to RFC3986.
//   For example: https://example.com/accessible/url.html
// Returns: Prints a sentence with verdict about whether the first user-agent
// is allowed or disallowed to access 'url' based on *specific* records
// targeting that user-agent in 'local_path_to_robotstxt'. If there are no
// specific rules targeting that user-agent, it falls back to the second
// user-agent.
//
#include <fstream>
#include <iostream>
#include <sstream>

#include "./robots.h"

int main(int argc, char** argv) {

  std::string robots_content = argv[1];

  std::string input_useragents = argv[2];
  std::vector<std::string> useragents;
  std::string ua;
  std::istringstream ss(input_useragents);

  // if we are given a comma-separated list of user agents, explode into a
  // vector
  while (std::getline(ss, ua, ',')) {
      useragents.push_back(ua);
  }
  googlebot::RobotsMatcher matcher;
  std::string url = argv[3];

  bool allowed;

  if (useragents.size() == 2) {
    // if we have multiple user agents, the first is the most specific
    std::vector<std::string> ua_tuple = {useragents[0], useragents[1]};

    allowed = matcher.AllowedByRobotsTuple(robots_content, &ua_tuple, url);
  } else {
    allowed = matcher.AllowedByRobots(robots_content, &useragents, url);
  }

  return allowed;
}
