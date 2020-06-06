include(GoogleTest)

add_executable(ClientTest
 main.cpp
 AlsaTest.cpp
 #ClientTest.cpp # This isn't quite stable yet
)

target_link_libraries(ClientTest PRIVATE
    project_options
    project_warnings
    AlsaInterface
    CONAN_PKG::gtest
)

target_include_directories(ClientTest PRIVATE
    $<TARGET_PROPERTY:AlsaInterface,INCLUDE_DIRECTORIES>
)

add_custom_command(TARGET ClientTest POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "${CMAKE_CURRENT_LIST_DIR}/../resources/*.raw"
      "${CMAKE_BINARY_DIR}/bin"
  COMMENT "Copying test audio files to ${CMAKE_BINARY_DIR}/bin"
)

# Skip tests that require user interaction
gtest_discover_tests(ClientTest)