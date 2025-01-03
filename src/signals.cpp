#include "themes.h"
#include <cstdlib>
#include <fmt/core.h>
#include <libproc.h>
#include <memory>
#include <signal.h>
#include <spdlog/spdlog.h>
#include <sys/event.h>
#include <unistd.h>

extern std::shared_ptr<spdlog::logger> logger;
extern pid_t process_to_track;

void signalHandler(int raised_signal) {
  logger->info("Got signal {}", raised_signal);
  spdlog::shutdown();
  resetScheme();
}

void setupProcessHook() {
  ::signal(SIGINT, signalHandler);
  ::signal(SIGPIPE, signalHandler);

  auto ppid = process_to_track;
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
