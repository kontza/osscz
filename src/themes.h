#pragma once
#include <string>

#define THEME_MARKER "TERMINAL_THEME"
#define MACOS_RESOURCE_DIR                                                     \
  "/Applications/Ghostty.app/Contents/Resources/ghostty"
#define LINUX_RESOURCE_DIR "/usr/share/ghostty"
#define PALETTE "palette"
#define TOML_NAME "scz.toml"

void resetScheme();
std::string getThemeName(std::string host_name);
void setSchemeForHost(std::string host_name);
bool shouldChangeTheme();
