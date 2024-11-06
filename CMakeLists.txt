cmake_minimum_required(VERSION 3.30)
project(osscz CXX)



add_executable(osscz src/osscz.cpp src/main.cpp)



install(TARGETS osscz DESTINATION "."
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        )
