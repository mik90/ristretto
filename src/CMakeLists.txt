add_library(
  AlsaInterface STATIC
    AlsaCapture.cpp
    AlsaPlayback.cpp
    AlsaInterface.cpp
    DecoderInterface.cpp
)

target_link_libraries(
  AlsaInterface PUBLIC project_options project_warnings
    CONAN_PKG::fmt
    CONAN_PKG::spdlog
    CONAN_PKG::boost
    asound
)


add_executable(
  AlsaInterfaceCli
   main.cpp)

target_link_libraries(
  AlsaInterfaceCli PRIVATE project_options project_warnings
    CONAN_PKG::docopt.cpp
    AlsaInterface
)