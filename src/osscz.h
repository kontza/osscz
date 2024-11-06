#pragma once

#include <vector>
#include <string>


#ifdef _WIN32
  #define OSSCZ_EXPORT __declspec(dllexport)
#else
  #define OSSCZ_EXPORT
#endif

OSSCZ_EXPORT void osscz();
OSSCZ_EXPORT void osscz_print_vector(const std::vector<std::string> &strings);
