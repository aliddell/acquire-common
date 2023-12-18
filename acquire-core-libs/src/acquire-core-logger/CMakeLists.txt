set(tgt acquire-core-logger)
add_library(${tgt} STATIC logger.h logger.c)
target_include_directories(${tgt} PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

install(TARGETS ${tgt})
