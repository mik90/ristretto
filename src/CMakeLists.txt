add_executable(AlsaInterface 
  main.cpp
  AlsaCapture.cpp
  AlsaPlayback.cpp
  AlsaInterface.cpp
  )

target_link_libraries(
  AlsaInterface PRIVATE project_options project_warnings
                CONAN_PKG::docopt.cpp
                CONAN_PKG::fmt
                CONAN_PKG::spdlog
                #${CONAN_LIB_DIRS_ABSEIL}/
                asound
                )

add_executable(DecoderTest 
  DecoderInterface.cpp
  )

target_link_libraries(
  DecoderTest PRIVATE project_options project_warnings
                CONAN_PKG::docopt.cpp
                CONAN_PKG::fmt
                CONAN_PKG::spdlog
                CONAN_PKG::boost
                )