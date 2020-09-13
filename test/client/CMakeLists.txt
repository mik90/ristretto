include(GoogleTest)

add_executable(ClientTest
 main.cpp
 AlsaTest.cpp
 #ClientTest.cpp # This isn't quite stable yet, requires a server
 UtilsTest.cpp
)

target_link_libraries(ClientTest PRIVATE
    project_options
    project_warnings
    AlsaInterface
<<<<<<< HEAD
    RistrettoClientLib
=======
    RistrettoClientCore
>>>>>>> master
    CONAN_PKG::gtest
)

target_include_directories(ClientTest PRIVATE
    $<TARGET_PROPERTY:AlsaInterface,INCLUDE_DIRECTORIES>
)

# Skip tests that require user interaction
gtest_discover_tests(ClientTest)