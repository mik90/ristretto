add_library(AlsaInterface STATIC
    AlsaCapture.cpp
    AlsaPlayback.cpp
    AlsaInterface.cpp
    ../Utils.cpp
)

# Turn spdlog onto the DEBUG level
target_compile_definitions(AlsaInterface PUBLIC
    SPDLOG_ACTIVE_LEVEL=1
)

target_include_directories(AlsaInterface PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
)

target_link_libraries(AlsaInterface PUBLIC
    project_options
    project_warnings
    # Alsa lib
    asound
    # Conan packages
    CONAN_PKG::fmt
    CONAN_PKG::spdlog
)


add_executable(AlsaInterfaceCli
  main.cpp
)


target_link_libraries(AlsaInterfaceCli PRIVATE
    project_options
    project_warnings
    # Internal lib
    AlsaInterface
    # Conan packages
    CONAN_PKG::docopt.cpp
)