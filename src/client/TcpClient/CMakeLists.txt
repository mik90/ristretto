add_executable(TcpClient
    TcpClient.cpp
    ../Utils.cpp
    main.cpp
)

# Turn spdlog onto the DEBUG level
target_compile_definitions(TcpClient PUBLIC
    SPDLOG_ACTIVE_LEVEL=1
)

target_include_directories(TcpClient PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
    ${CMAKE_CURRENT_LIST_DIR}/AlsaInterface
)

target_link_libraries(TcpClient PUBLIC
    project_options
    project_warnings
    # This could be an optional dependency with conan but it's not worth the effort
    boost_system
    # Conan packages
    CONAN_PKG::fmt
    CONAN_PKG::spdlog
    # Internal
    AlsaInterface

)