include(cmake/wsl.cmake)

if(WIN32)
    add_subdirectory(win32)
elseif(APPLE)
    add_subdirectory(osx)
elseif(LINUX)
    add_subdirectory(linux)
endif()

install(TARGETS acquire-core-platform)
