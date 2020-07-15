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
// Usage:
//     robots_main <local_path_to_robotstxt> <user_agent> <url>
// Arguments:
// local_path_to_robotstxt: local path to a file containing robots.txt records.
//   For example: /home/users/username/robots.txt
// user_agent: a token to be matched against records in the robots.txt.
//   For example: Googlebot
// url: a url to be matched against records in the robots.txt. The URL must be
// %-encoded according to RFC3986.
//   For example: https://example.com/accessible/url.html
// Output: Prints a sentence with verdict about whether 'user_agent' is allowed
// to access 'url' based on records in 'local_path_to_robotstxt'.
// Return code:
//   0 when the url is ALLOWED for the user_agent.
//   1 when the url is DISALLOWED for the user_agent.
//   2 when --help is requested or if there is something invalid in the flags
//   passed.
//
#include <fstream>
#include <iostream>

#include "robots.h"

bool LoadFile(const std::string& filename, std::string* result) {
  std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
  if (file.is_open()) {
    size_t size = file.tellg();
    std::vector<char> buffer(size);
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), size);
    file.close();
    if (!file) return false;  // file reading error (failbit or badbit).
    result->assign(buffer.begin(), buffer.end());
    return true;
  }
  return false;
}

void ShowHelp(int argc, char** argv) {
  std::cerr << "Shows whether the given user_agent and URI combination"
            << " is allowed or disallowed by the given robots.txt file. "
            << std::endl
            << std::endl;
  std::cerr << "Usage: " << std::endl
            << "  " << argv[0] << " <robots.txt filename> <user_agent> <URI>"
            << std::endl
            << std::endl;
  std::cerr << "The URI must be %-encoded according to RFC3986." << std::endl
            << std::endl;
  std::cerr << "Example: " << std::endl
            << "  " << argv[0] << " robots.txt FooBot http://example.com/foo"
            << std::endl;
}

int main(int argc, char** argv) {
  std::string filename = argc >= 2 ? argv[1] : "";
  if (filename == "-h" || filename == "-help" || filename == "--help") {
    ShowHelp(argc, argv);
    return 2;
  }
  if (argc != 4) {
    std::cerr << "Invalid amount of arguments. Showing help." << std::endl
              << std::endl;
    ShowHelp(argc, argv);
    return 2;
  }
  std::string robots_content;
  if (!(LoadFile(filename, &robots_content))) {
    std::cerr << "failed to read file \"" << filename << "\"" << std::endl;
    return 2;
  }

  std::string user_agent = argv[2];
  std::vector<std::string> user_agents(1, user_agent);
  googlebot::RobotsMatcher matcher;
  std::string url = argv[3];
  bool allowed = matcher.AllowedByRobots(robots_content, &user_agents, url);

  std::cout << "user-agent '" << user_agent << "' with URI '" << argv[3]
            << "': " << (allowed ? "ALLOWED" : "DISALLOWED") << std::endl;
  if (robots_content.empty()) {
    std::cout << "notice: robots file is empty so all user-agents are allowed"
              << std::endl;
  }

  return allowed ? 0 : 1;
}
