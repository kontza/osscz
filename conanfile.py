from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout


class ossczRecipe(ConanFile):
    name = "osscz"
    version = "0.1.0"
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

    def requirements(self):
        self.requires("fmt/11.0.2")
