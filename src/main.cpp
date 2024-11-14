#include "cmd.h"
#include "themes.h"
#include "version.h"
#include <cstdlib>
#include <filesystem>
#include <fmt/core.h>
#include <memory>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/spdlog.h>
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
    auto reset_theme = std::string{"RESET-THEME"};
    if (host_name == reset_scheme || host_name == reset_theme) {
      resetScheme();
    } else {
      setSchemeForHost(host_name);
    }
  }

  // Done.
  logger->info("All done, TTFN!");
  return 0;
}
