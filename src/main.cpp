#include "osscz.h"
#include <vector>
#include <string>

int main() {
    osscz();

    std::vector<std::string> vec;
    vec.push_back("test_package");

    osscz_print_vector(vec);
}
