from conans import ConanFile, CMake

class RistrettoConan(ConanFile):
    author = "Mike Kaliman"
    version = "0.1"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake", "gcc", "txt"
    options = {"build_server" : [True, False],
               "build_client" : [True, False]}
    default_options = {"build_server" : False,
                       "build_client" : True}

    def requirements(self):
        self.requires("gtest/1.10.0")
        self.requires("docopt.cpp/0.6.2")
        self.requires("fmt/6.2.0")
        self.requires("spdlog/1.5.0")
        self.requires("nlohmann_json/3.8.0")
        # Any other dependencies are handled by Dockerfiles since both the server
        # and client are containerized


    def build(self):
        # Run "conan install . --build missing" to build all missing dependencies
        cmake = CMake(self)
        if self.options["build_server"]:
            cmake.definitions["BUILD_SERVER"] = "ON"
        else:
            cmake.definitions["BUILD_SERVER"] = "OFF"

        if self.options["build_client"]:
            cmake.definitions["BUILD_CLIENT"] = "ON"
        else:
            cmake.definitions["BUILD_CLIENT"] = "OFF"
        cmake.generator = "Ninja"
        cmake.configure()
        cmake.build()
