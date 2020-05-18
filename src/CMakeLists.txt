add_subdirectory(client)

if (BUILD_SERVER)
    add_subdirectory(server)
endif()