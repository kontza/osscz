#include "themes.h"
#include "trim.h"
#include "version.h"
#include <cstdlib>
#include <filesystem>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <memory>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/spdlog.h>
#include <string>
#include <sys/_types/_pid_t.h>
#include <sys/event.h>
#include <unistd.h>

#ifdef NDEBUG
#define BUILD ""
#else
#define BUILD "-DBG"
#endif

#define LOG_NAME "ssh_colouriser.log"
std::shared_ptr<spdlog::logger> logger;

spdlog::filename_t getLogFilename() {
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

int main(int argc, char *argv[]) {
  // Sanity check on command line arguments.
  if (argc < 2) {
    fmt::println("{} v{}{}", APP_NAME, APP_VERSION, BUILD);
    spdlog::critical("Not enough arguments!");
    return 1;
  }

  // Set up logging to file.
  logger = spdlog::daily_logger_st(APP_NAME, getLogFilename(), 3, 14, false, 5);
  logger->set_level(spdlog::level::trace);
  logger->set_error_handler(
      [](const std::string &msg) { throw std::runtime_error(msg); });
  spdlog::set_default_logger(logger);
  std::time_t now = std::time(nullptr);
  logger->info("=== {:%Y-%m-%d} {:%H:%M:%S}", fmt::localtime(now),
               fmt::localtime(now));
  logger->info("Welcome my son, welcome to the machine!");

  // Change or reset?
  std::string arguments;
  auto should_reset = false;
  auto reset_scheme = std::string{"RESET-SCHEME"};
  auto reset_theme = std::string{"RESET-THEME"};
  for (int i = 1; i < argc; i++) {
    if (argv[i] != nullptr) {
      if (argv[i] == reset_scheme || argv[i] == reset_theme) {
        should_reset = true;
      }
      arguments += argv[i];
      arguments += " ";
    }
  }
  auto theme_name = getThemeName(arguments);
  if (should_reset) {
    resetScheme();
  } else {
    setScheme(theme_name);
  }

  // Done.
  logger->info("All done, TTFN!");
  return 0;
}
