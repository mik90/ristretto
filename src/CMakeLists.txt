add_executable(AlsaInterface 
  main.cpp
  AlsaCapture.cpp
  AlsaPlayback.cpp
  AlsaInterface.cpp
  DecoderInterface.cpp
  )
target_link_libraries(
  AlsaInterface PRIVATE project_options project_warnings
                CONAN_PKG::docopt.cpp
                CONAN_PKG::fmt
                CONAN_PKG::spdlog
                CONAN_PKG::abseil
                CONAN_PKG::boost
                asound
                )
