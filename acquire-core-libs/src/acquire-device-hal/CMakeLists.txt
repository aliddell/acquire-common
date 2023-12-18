set(tgt acquire-device-hal)
add_library(${tgt} STATIC
        device/hal/camera.h
        device/hal/camera.c
        device/hal/driver.h
        device/hal/driver.c
        device/hal/device.manager.h
        device/hal/device.manager.cpp
        device/hal/loader.h
        device/hal/loader.c
        device/hal/experimental/stage.axis.h
        device/hal/experimental/stage.axis.c
        device/hal/storage.h
        device/hal/storage.c
)
target_sources(${tgt} PUBLIC FILE_SET HEADERS
        BASE_DIRS "${CMAKE_CURRENT_LIST_DIR}"
        FILES
        device/hal/device.manager.h
)
target_link_libraries(${tgt} PRIVATE
        acquire-device-kit
        acquire-core-logger
        acquire-core-platform
)
target_include_directories(${tgt} PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

install(TARGETS ${tgt} FILE_SET HEADERS)
