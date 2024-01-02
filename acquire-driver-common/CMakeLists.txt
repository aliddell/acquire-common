project(acquire-driver-common)

include(../cmake/msvc.cmake)
include(../cmake/git-versioning.cmake)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(src)
add_subdirectory(tests)
