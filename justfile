# Default target: list all targets
alpha:
    just -l

# R-1. Generate build/Release configuration
conan-release:
    conan install .

# R-2. Configure build/Release
cmake-release: conan-release
    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 --preset conan-release
    ln -sf ./build/Release/compile_commands.json
    cp ./build/Release/version.h ./src/

# R-3. Build release
release:
    cmake --build --preset conan-release
    cp ./build/Release/osscz $HOME/.local/bin/

# D-1. Generate build/Debug configuration
conan-debug:
    conan install . --profile Debug

# D-2. Configure build/Debug
cmake-debug: conan-debug
    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 --preset conan-debug
    ln -sf ./build/Debug/compile_commands.json
    cp ./build/Debug/version.h ./src/

# D-3. Build debug
debug:
    cmake --build --preset conan-debug

