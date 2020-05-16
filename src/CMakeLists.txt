add_library(
  AlsaInterface STATIC
    AlsaCapture.cpp
    AlsaPlayback.cpp
    AlsaInterface.cpp
    DecoderClient.cpp
    Utils.cpp
)
# Turn spdlog onto the DEBUG level
target_compile_definitions(AlsaInterface PUBLIC SPDLOG_ACTIVE_LEVEL=1)

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