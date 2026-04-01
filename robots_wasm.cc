#include <emscripten/bind.h>

#include "robots.h"

// Returns true if the given user agent is allowed to crawl the URL given the
// robots content Otherwise, returns false. We also need to make sure our
// function gets exported as IsAllowed.
bool IsAllowed(std::string user_agent, std::string url,
               std::string robots_content) asm("IsAllowed");
bool IsAllowed(std::string user_agent, std::string url,
               std::string robots_content) {
  googlebot::RobotsMatcher matcher;
  std::vector<std::string> user_agents(1, user_agent);
  return matcher.AllowedByRobots(robots_content, &user_agents, url);
}

// Create a binding so we can call the function from JavaScript.
// Example: Module.IsAllowed('googlebot', 'https://example.com/test',
// 'user-agent: *\ndisallow: /') // will return false
EMSCRIPTEN_BINDINGS(my_module) {
  emscripten::function("IsAllowed", &IsAllowed);
}
