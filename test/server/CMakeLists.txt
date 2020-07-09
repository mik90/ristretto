include(GoogleTest)

add_executable(ServerTest
 main.cpp
 ServerTest.cpp
)

target_link_libraries(ServerTest PRIVATE
    project_options
    project_warnings
    RistrettoServerCore
    CONAN_PKG::gtest
)

target_include_directories(ServerTest PRIVATE
    $<TARGET_PROPERTY:RistrettoServerCore,INCLUDE_DIRECTORIES>
)

# Skip tests that require user interaction
gtest_discover_tests(ServerTest)