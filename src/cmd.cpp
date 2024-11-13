#include "cmd.h"
#include <array>
#include <fmt/core.h>
#include <memory>

std::string runCommand(std::string cmd) {
  std::array<char, 1024> buffer;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"),
                                                pclose);
  std::string result{""};
  if (!pipe) {
    auto const msg = fmt::format("Failed to popen() on '{}'", cmd);
    throw std::runtime_error(msg);
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != NULL) {
    result += buffer.data();
  }
  return std::move(result);
}

std::string pidToCommandLine(pid_t pid) {
  auto result = runCommand(fmt::format("/bin/ps -eo args= {}", pid));
  for (auto it = result.rbegin(); it != result.rend(); it++) {
    if (*it == '\n') {
      *it = '\0';
    }
  }
  return std::move(result);
}
