include(GoogleTest)

add_executable(ServerTest
 main.cpp
 ServerTest.cpp
)

target_link_libraries(ServerTest PRIVATE
    project_options
    project_warnings
<<<<<<< HEAD
    RistrettoServerLib
    CONAN_PKG::gtest
)

=======
    RistrettoServerCore
    CONAN_PKG::gtest
)

target_include_directories(ServerTest PRIVATE
    $<TARGET_PROPERTY:RistrettoServerCore,INCLUDE_DIRECTORIES>
)

>>>>>>> master
# Skip tests that require user interaction
gtest_discover_tests(ServerTest)