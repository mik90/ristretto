include(GoogleTest)

add_executable(ClientTest ClientTest.cpp)

target_link_libraries(
  ClientTest PRIVATE project_options project_warnings
    AlsaInterface
    CONAN_PKG::gtest
)

target_include_directories(
  ClientTest PRIVATE
   ${client_SOURCE_DIR}
)
add_custom_command(TARGET ClientTest POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "${CMAKE_CURRENT_LIST_DIR}/../resources/*.raw"
      "${CMAKE_BINARY_DIR}/test/client"
  COMMENT "Copying test audio files to ${CMAKE_BINARY_DIR}/test/client"
)

# Skip tests that require user interaction
gtest_discover_tests(ClientTest)