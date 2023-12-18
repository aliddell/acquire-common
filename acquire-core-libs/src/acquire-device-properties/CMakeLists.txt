set(tgt acquire-device-properties)
add_library(${tgt} STATIC
        device/props/device.c
        device/props/storage.c
        device/props/components.c
)
target_sources(${tgt} PUBLIC FILE_SET HEADERS
        BASE_DIRS "${CMAKE_CURRENT_LIST_DIR}"
        FILES
        device/props/camera.h
        device/props/components.h
        device/props/device.h
        device/props/experimental/signals.h
        device/props/experimental/stage.axis.h
        device/props/metadata.h
        device/props/storage.h
)
target_include_directories(${tgt} PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_link_libraries(${tgt} PRIVATE
        acquire-core-logger
        acquire-core-platform
)

install(TARGETS ${tgt} FILE_SET HEADERS)
