#include "version.h"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fmt/core.h>
#include <memory>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/spdlog.h>
#include <sys/_types/_pid_t.h>
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

std::string pidToCommandLine(pid_t pid) {
  auto const cmd = fmt::format("/bin/ps -eo args= {}", pid);
  std::array<char, 1024> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"),
                                                pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  } else {
    spdlog::info("pipe succeeded");
  }
  for (auto ptr = fgets(buffer.data(), buffer.size(), pipe.get());
       ptr != NULL || ::feof(pipe.get()) == 0;
       ptr = fgets(buffer.data(), buffer.size(), pipe.get())) {
    spdlog::info("received {}", ptr);
    result += ptr;
    spdlog::info("result so far {}", result);
    if (feof(pipe.get())) {
      spdlog::info("EOF");
      pclose(pipe.get());
      break;
    }
  }
  return result;
}

bool shouldChangeTheme() {
  auto const ppid = getppid();
  auto const parent_command_line = pidToCommandLine(ppid);
  logger->info("Parent command line: {}", parent_command_line);
  return false;
}

int main(int argc, char *argv[]) {
  // Sanity check on command line arguments.
  if (argc != 2) {
    fmt::println("{} v{}{}", APP_NAME, APP_VERSION, BUILD);
    spdlog::critical("Gimme a single SSH host name to work on!");
    return 1;
  }

  // Set up logging to file.
  auto logger = spdlog::daily_logger_mt(APP_NAME, get_log_filename(), 3, 14);
  std::time_t now = std::time(nullptr);
  logger->info("=== {:%Y-%m-%d} {:%H:%M:%S}", fmt::localtime(now),
               fmt::localtime(now));
  logger->info("Welcome my son, welcome to the machine!");

  // Should we change the theme?
  auto const do_work_or_failed = shouldChangeTheme();

  // Done.
  logger->info("All done, TTFN!");
  return 0;
}
