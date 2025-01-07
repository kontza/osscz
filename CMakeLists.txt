cmake_minimum_required(VERSION 3.30)
project(osscz LANGUAGES CXX VERSION 1.5.3)
configure_file(version.h.in version.h)

find_package(spdlog REQUIRED)
find_package(toml11 REQUIRED)

add_executable(${PROJECT_NAME}
        src/trim.cpp
        src/themes.cpp
        src/cmd.cpp
        src/env-expand.cpp
        src/main.cpp)
target_link_libraries(${PROJECT_NAME} spdlog::spdlog)
target_link_libraries(${PROJECT_NAME} toml11::toml11)
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_BINARY_DIR}")

install(TARGETS ${PROJECT_NAME} DESTINATION "."
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        )
