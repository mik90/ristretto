add_executable(AlsaInterface 
  main.cpp
  AlsaCapture.cpp
  AlsaPlayback.cpp
  AlsaInterface.cpp
  )
target_link_libraries(
  AlsaInterface PRIVATE project_options project_warnings CONAN_PKG::docopt.cpp
                CONAN_PKG::fmt CONAN_PKG::spdlog CONAN_PKG::abseil
                asound
                )
