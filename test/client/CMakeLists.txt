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

# Skip tests that require user interaction
gtest_discover_tests(ClientTest)