cmake_minimum_required(VERSION 3.30)
project(osscz LANGUAGES CXX VERSION 0.1.0)
configure_file(version.h.in version.h)

find_package(spdlog REQUIRED)


add_executable(${PROJECT_NAME} src/osscz.cpp src/main.cpp)
target_link_libraries(${PROJECT_NAME} spdlog::spdlog)
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_BINARY_DIR}")

install(TARGETS ${PROJECT_NAME} DESTINATION "."
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        )
