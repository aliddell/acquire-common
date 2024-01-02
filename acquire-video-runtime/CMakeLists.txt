project(acquire-video-runtime)

include(../cmake/msvc.cmake)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(src)
add_subdirectory(tests)
