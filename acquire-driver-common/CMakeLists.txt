project(acquire-driver-common)

include(../cmake/msvc.cmake)
include(../cmake/git-versioning.cmake)

add_subdirectory(src)
add_subdirectory(tests)
