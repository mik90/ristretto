include(GoogleTest)

add_executable(ClientTest ClientTest.cpp)

target_link_libraries(
  ClientTest PRIVATE project_options project_warnings
    AlsaInterface
    CONAN_PKG::gtest
)

target_include_directories(
  ClientTest PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Skip tests that require user interaction
gtest_discover_tests(ClientTest 
  EXTRA_ARGS --gtest_filter=-*USER_INPUT_*
)