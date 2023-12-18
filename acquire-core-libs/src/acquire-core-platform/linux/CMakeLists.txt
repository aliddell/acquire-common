set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(tgt acquire-core-platform)
add_library(${tgt} STATIC
        platform.h
        platform.c)
target_include_directories(${tgt} PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_link_libraries(${tgt} PRIVATE Threads::Threads acquire-core-logger)