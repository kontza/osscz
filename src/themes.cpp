#include "themes.h"
#include "cmd.h"
#include "env-expand.h"
#include "trim.h"
#include <cmath>
#include <filesystem>
#include <fmt/core.h>
#include <fstream>
#include <libproc.h>
#include <map>
#include <ranges>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <toml.hpp>
#include <unistd.h>

#define LOG_ANSI_FMT "\\x1b]{}{}\\x07"
#define PRINT_ANSI_FMT "\x1b]{}{}\x07"
#define LOG_FMT "'{}': Would use '{}'"
#define TOML "TOML"
#define SSH_MARKER "ssh "
#define LAUNCHD_PID 1

extern std::shared_ptr<spdlog::logger> logger;
pid_t process_to_track;

static std::map<std::string, std::string> patterns = {
    {PALETTE, "4;"},
    {"foreground", "10;"},
    {"background", "11;"},
    {"cursor-color", "12;"},
};

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

std::string getThemeName(std::string arguments) {
  logger->info("Scanning SSH config for {} in '{}'...", THEME_MARKER,
               arguments);
  auto result = runCommand(fmt::format("ssh -G {}", arguments));
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

void printAnsiEscape(std::string line, std::string prefix, std::string value,
                     int color_index) {
  logger->error("VALUE = {}", value);
  // Escape backslashes for logging.
  auto value_to_print = value;
  if (color_index >= 0) {
    value_to_print = fmt::format("{};{}", color_index, value);
  }
  auto to_log = fmt::format(LOG_ANSI_FMT, prefix, value_to_print);
  logger->info(LOG_FMT, line, to_log);
  // Print out ANSI escape codes to set colors.
  fmt::print(PRINT_ANSI_FMT, prefix, value);
}

void handleTomlTheme(std::string theme_name) {
  auto toml = toml::parse(envExpand(theme_name));
  auto ansi = toml::find<std::vector<std::string>>(toml, "colors", "ansi");
  auto brights =
      toml::find<std::vector<std::string>>(toml, "colors", "brights");
  auto foreground = toml::find<std::string>(toml, "colors", "foreground");
  auto background = toml::find<std::string>(toml, "colors", "background");
  std::string cursor{};
  try {
    cursor = toml::find<std::string>(toml, "colors", "cursor_fg");
  } catch (std::out_of_range) {
    logger->info("compose_cursor not found");
  }
  int end_index = fminl(ansi.size(), 8);

  for (int color_index = 0; color_index < ansi.size(); color_index++) {
    printAnsiEscape(TOML, patterns[PALETTE], ansi[color_index], color_index);
  }
  end_index = fminl(brights.size(), 8);
  for (int color_index = 0; color_index < brights.size(); color_index++) {
    printAnsiEscape(TOML, patterns[PALETTE], brights[color_index],
                    color_index + 8);
  }
  printAnsiEscape(TOML, patterns["foreground"], foreground, -1);
  printAnsiEscape(TOML, patterns["background"], background, -1);
  if (!cursor.empty()) {
    printAnsiEscape(TOML, patterns["cursor-color"], cursor, -1);
  }
}

void handleGhosttyTheme(std::string theme_name) {
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
      //   background = #202746
      // Split line from first '='.
      auto eq_pos = line.find('=');
      if (eq_pos == std::string::npos) {
        continue;
      }
      // 'palette' or 'background'
      auto setting_name = line.substr(0, eq_pos);
      rtrim(setting_name);
      // '15=#f5f7ff' or '#202746'
      auto setting = line.substr(1 + eq_pos);
      ltrim(setting);
      // Now we have either '15=#f5f7ff', or '#202746'.
      // Get keys from patterns map.
      auto kv = std::views::keys(patterns);
      std::vector<std::string> keys{kv.begin(), kv.end()};
      // Iterate over those map keys.
      for (auto const &pattern : keys) {
        // Does the current map key match the current line's setting name?
        if (pattern == setting_name) {
          // Convert '15=#f5f7ff' to '15;#f5f7ff'.
          // '#202746' will be left as is.
          std::replace(setting.begin(), setting.end(), '=', ';');
          // Sanity check: do we have a value to set?
          if (setting.length() > 0) {
            printAnsiEscape(line, patterns[setting_name], setting, -1);
          }
        }
      }
    }
    file.close();
  } else {
    logger->info("Theme file not open!");
  }
}

void setScheme(std::string theme_name) {
  if (theme_name.length()) {
    // Is this a TOML file?
    std::filesystem::path theme_path(theme_name);
    if (theme_path.extension().string() == ".toml") {
      handleTomlTheme(theme_name);
    } else {
      handleGhosttyTheme(theme_name);
    }
  } else {
    logger->info("No {} found for", THEME_MARKER);
  }
}
