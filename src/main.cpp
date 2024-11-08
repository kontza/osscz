#include "version.h"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fmt/core.h>
#include <iosfwd>
#include <map>
#include <memory>
#include <ranges>
#include <regex>
#include <signal.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/spdlog.h>
#include <string>
#include <sys/_types/_pid_t.h>
#include <sys/event.h>
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
#define THEME_MARKER "TERMINAL_THEME"
#define PALETTE "palette"
#define MACOS_RESOURCE_DIR                                                     \
  "/Applications/Ghostty.app/Contents/Resources/ghostty"
#define LINUX_RESOURCE_DIR "/usr/share/ghostty"
std::shared_ptr<spdlog::logger> logger;
static std::map<std::string, std::string> patterns = {
    {PALETTE, "4;"},
    {"foreground", "10;#"},
    {"background", "11;#"},
    {"cursor-color", "12;#"},
};

// trim from start (in place)
inline void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

// trim from end (in place)
inline void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

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

void resetScheme() {
  logger->info("Going to reset scheme");
  int counter = 0;
  for (int counter = 0; counter < 16; counter++) {
    fmt::print("\x1b]104;{}\x07", counter);
  }
  fmt::print("\x1b]110\x07");
  fmt::print("\x1b]111\x07");
  fmt::print("\x1b]112\x07");
}

void signalHandler(int raised_signal) {
  logger->info("Got signal {}", raised_signal);
  resetScheme();
}

void setupProcessHook() {
  ::signal(SIGINT, signalHandler);
  ::signal(SIGPIPE, signalHandler);

  auto ppid = getppid();
  auto fpid = fork();
  if (fpid != 0) {
    logger->info("Forked successfully");
    exit(EXIT_SUCCESS);
  }

  struct kevent kev;
  EV_SET(&kev, ppid, EVFILT_PROC, EV_ADD, NOTE_EXIT, 0, 0);

  auto kq = kqueue();
  if (kq < 0) {
    logger->info("Failed to acquire kqueue");
    exit(EXIT_FAILURE);
  }

  auto kret = kevent(kq, &kev, 1, NULL, 0, NULL);
  if (kret < 0) {
    logger->info("Failed with the first kevent call");
    exit(EXIT_FAILURE);
  }

  struct timespec timeout;
  timeout.tv_sec = 8 * 60 * 60;
  timeout.tv_nsec = 0;

  kret = kevent(kq, NULL, 0, &kev, 1, &timeout);
  if (kret < 0) {
    logger->info("Failed with the second kevent call");
    exit(EXIT_FAILURE);
  }
  if (kret > 0) {
    resetScheme();
  }
}

std::string getThemeName(std::string host_name) {
  logger->info("Scanning SSH config for {}", THEME_MARKER);
  auto result = runCommand(fmt::format("ssh -G {}", host_name));
  std::stringstream ss(result);
  std::string line;
  char delimiter = '\n';
  std::string theme_marker{fmt::format("setenv {}", THEME_MARKER)};
  while (getline(ss, line, delimiter)) {
    ltrim(line);
    if (!line.starts_with('#') && line.contains(theme_marker)) {
      logger->info("{} found", THEME_MARKER);
      auto eq_pos = line.find('=');
      if (eq_pos != std::string::npos) {
        auto theme_name = line.substr(1 + eq_pos);
        logger->info("Theme name: '{}'", theme_name);
        return std::move(theme_name);
      }
    }
  }
  return "";
}

void setSchemeForHost(std::string host_name) {
  auto const theme_name = getThemeName(host_name);
  if (theme_name.length()) {
    auto res_dir_name_ptr = std::getenv("GHOSTTY_RESOURCES_DIR");
    std::string res_dir_name;
    if (res_dir_name_ptr == nullptr) {
#ifdef __APPLE__
      res_dir_name = MACOS_RESOURCE_DIR;
#else
      res_dir_name = LINUX_RESOURCE_DIR;
#endif // __APPLE__
    } else {
      res_dir_name = res_dir_name_ptr;
    }
    logger->info("Reading Ghostty resources from '{}'", res_dir_name);
    std::filesystem::path res_dir{res_dir_name};
    auto theme_path{res_dir / "themes" / theme_name};
    logger->info("Theme file '{}'", theme_path.string());
    std::ifstream file(theme_path);
    if (file.is_open()) {
      std::string line;
      // Scan theme file.
      while (std::getline(file, line)) {
        ltrim(line);
        // Bypass empty lines, and comment lines.
        if (!line.length() || line.starts_with('#')) {
          continue;
        }
        // Sample lines:
        //   palette = 15=#f5f7ff
        //   background = 202746
        // Split line from first '='.
        auto eq_pos = line.find('=');
        if (eq_pos == std::string::npos) {
          continue;
        }
        // 'palette' or 'background'
        auto setting_name = line.substr(0, eq_pos);
        rtrim(setting_name);
        // '15=#f5f7ff' or '202746'
        auto setting = line.substr(1 + eq_pos);
        ltrim(setting);
        // Now we have either '15=#f5f7ff', or '202746'.
        // Get keys from patterns map.
        auto kv = std::views::keys(patterns);
        std::vector<std::string> keys{kv.begin(), kv.end()};
        std::string to_log{""};
        // Iterate over those map keys.
        for (auto const &pattern : keys) {
          // Does the current map key match the current line's setting name?
          if (pattern == setting_name) {
            // Convert '15=#f5f7ff' to '15;#f5f7ff'.
            // '202746' will be left as is.
            std::replace(setting.begin(), setting.end(), '=', ';');
            // Sanity check: do we have a value to set?
            if (setting.length() > 0) {
              // Escape backslashes for logging.
              to_log = fmt::format("\\x1b]{}{}\\x07", patterns[setting_name],
                                   setting);
              logger->info("'{}': Would use '{}'", line, to_log);
              // Print out ANSI escape codes to set colors.
              fmt::print("\x1b]{}{}\x07", patterns[setting_name], setting);
            }
          }
        }
      }
      file.close();
    } else {
      logger->info("Theme file not open!");
    }
  } else {
    logger->info("No {} found for '{}'", THEME_MARKER, host_name);
  }
  setupProcessHook();
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
  auto result = runCommand(fmt::format("/bin/ps -eo args= {}", pid));
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
      setSchemeForHost(host_name);
    }
  }

  // Done.
  logger->info("All done, TTFN!");
  return 0;
}
