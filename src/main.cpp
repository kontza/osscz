#include "version.h"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fmt/core.h>
#include <memory>
#include <regex>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/spdlog.h>
#include <sys/_types/_pid_t.h>
#include <toml.hpp>
#include <unistd.h>
#include <vector>

#ifdef NDEBUG
#define BUILD ""
#else
#define BUILD "-DBG"
#endif

#define LOG_NAME "ssh_colouriser.log"
#define TOML_NAME "scz.toml"
std::shared_ptr<spdlog::logger> logger;

void resetScheme() {
  logger->info("Going to reset scheme");
  int counter = 0;
  for (int counter = 0; counter < 15; counter++) {
    fmt::print("\x1b]104;{}\x07", '0' + counter);
  }
  fmt::print("\x1b]110\x07");
  fmt::print("\x1b]111\x07");
  fmt::print("\x1b]112\x07");
}

spdlog::filename_t get_log_filename() {
  auto tmp_dir_ptr = std::getenv("TMPDIR");
  std::string tmp_dir;
  if (tmp_dir_ptr == nullptr) {
    tmp_dir = std::string{"/tmp"};
  } else {
    tmp_dir = tmp_dir_ptr;
  }
  const std::filesystem::path ret_val = tmp_dir;
  return ret_val / LOG_NAME;
}

std::string pidToCommandLine(pid_t pid) {
  auto const cmd = fmt::format("/bin/ps -eo args= {}", pid);
  std::array<char, 1024> buffer;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"),
                                                pclose);
  std::string result{""};
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != NULL) {
    result += buffer.data();
  }
  for (auto it = result.rbegin(); it != result.rend(); it++) {
    if (*it == '\n') {
      *it = '\0';
    }
  }

  return std::move(result);
}

bool shouldChangeTheme() {
  auto const ppid = getppid();
  auto parent_command_line = pidToCommandLine(ppid);
  logger->info("Parent, {}, command line: '{}'", ppid, parent_command_line);
  auto config_home_ptr = std::getenv("XDG_CONFIG_HOME");
  std::string config_home;
  if (config_home_ptr == nullptr) {
    config_home = std::getenv("HOME");
  } else {
    config_home = config_home_ptr;
  }
  std::filesystem::path config_path = config_home;
  std::filesystem::path toml_path = config_path / TOML_NAME;
  auto toml = toml::parse(toml_path.string());
  auto bypasses = toml::find<std::vector<std::string>>(toml, "bypasses");
  for (auto bypass : bypasses) {
    std::regex checker(bypass, std::regex_constants::ECMAScript);
    logger->info("Checking against '{}'", bypass);
    if (std::regex_search(parent_command_line, checker)) {
      logger->info("Matched");
      return false;
    }
  }
  logger->info("None of the bypasses matched");
  return true;
}

int main(int argc, char *argv[]) {
  // Sanity check on command line arguments.
  if (argc != 2) {
    fmt::println("{} v{}{}", APP_NAME, APP_VERSION, BUILD);
    spdlog::critical("Gimme a single SSH host name to work on!");
    return 1;
  }

  // Set up logging to file.
  logger = spdlog::daily_logger_mt(APP_NAME, get_log_filename(), 3, 14);
  std::time_t now = std::time(nullptr);
  logger->info("=== {:%Y-%m-%d} {:%H:%M:%S}", fmt::localtime(now),
               fmt::localtime(now));
  logger->info("Welcome my son, welcome to the machine!");

  // Should we change the theme?
  if (shouldChangeTheme()) {
    auto host_name = argv[1];
    auto reset_scheme = std::string{"RESET-SCHEME"};
    if (host_name == reset_scheme) {
      resetScheme();
    } else {
    }
  }

  // Done.
  logger->info("All done, TTFN!");
  return 0;
}
