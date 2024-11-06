#include "version.h"
#include <fmt/core.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fmt::println("{} v{}\n\nGimme a single SSH host name to work on!", APP_NAME,
                 APP_VERSION);
  }
  return 0;
}
