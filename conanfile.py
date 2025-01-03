import sys
from typing import Self

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout


def get_version() -> str:
    with open("CMakeLists.txt", "r") as cmakelists:
        for line in cmakelists.readlines():
            if line.strip().startswith("project") and "VERSION" in line:
                capture = False
                version = ""
                for part in line.split():
                    if part == "VERSION":
                        capture = True
                        continue
                    if capture:
                        version = part.replace(")", "")
                        break
                print(f"Found version {version} from CMakeLists.txt")
                return version
    print("Did not find application version from CMakeLists.txt!", file=sys.stderr)
    return "N/A"


class ossczRecipe(ConanFile):
    name = "osscz"
    version = get_version()
    package_type = "application"

    # Optional metadata
    license = "MIT"
    author = "Juha Ruotsalainen <juha.ruotsalainen@iki.fi>"
    url = "https://github.com/kontza/osscz.git"
    description = "Set Ghostty pane background colour on SSH connection"
    topics = ("ghostty", "ssh")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "src/*"

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def requirements(self: Self):
        self.requires("spdlog/1.15.0")
        self.requires("toml11/4.2.0")
        self.requires("fmt/11.0.2")
