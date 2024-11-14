#include <memory>
#include <spdlog/spdlog.h>
#include <string>

extern std::shared_ptr<spdlog::logger> logger;

std::string envExpand(std::string input_string) {
  std::string expanded{};
  std::string to_expand{};
  bool capture = false;
  for (auto it = input_string.begin(); it != input_string.end(); it++) {
    if (*it == '$') {
      capture = true;
      continue;
    }
    if (!capture) {
      expanded += *it;
    } else {
      if (isupper(*it) || isdigit(*it) || *it == '_') {
        to_expand += *it;
      } else {
        capture = false;
        auto value_from_env = std::getenv(to_expand.c_str());
        to_expand.clear();
        logger->info("Extracted env var {} = {}", to_expand, value_from_env);
        if (value_from_env != nullptr) {
          expanded += value_from_env;
        }
        expanded += *it;
      }
    }
  }
  logger->info("Expanded string: {}", expanded);
  return std::move(expanded);
}
