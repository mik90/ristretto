include(GoogleTest)

add_executable(ServerTest
 main.cpp
 ServerTest.cpp
)

target_link_libraries(ServerTest PRIVATE
    project_options
    project_warnings
    RistrettoServerLib
    CONAN_PKG::gtest
)

# Skip tests that require user interaction
gtest_discover_tests(ServerTest)