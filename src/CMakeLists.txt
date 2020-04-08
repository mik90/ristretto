add_executable(AlsaCapture 
  AlsaCapture.cpp
  AlsaInterface.cpp
  )
target_link_libraries(
  AlsaCapture PRIVATE project_options project_warnings CONAN_PKG::docopt.cpp
                CONAN_PKG::fmt CONAN_PKG::spdlog CONAN_PKG::abseil
                asound
                )
