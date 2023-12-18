set(tgt acquire-device-kit)
add_library(${tgt} INTERFACE
        device/kit/driver.h
        device/kit/camera.h
        device/kit/experimental/signals.h
        device/kit/experimental/stage.axis.h
        device/kit/storage.h
)
target_link_libraries(${tgt} INTERFACE acquire-device-properties)
target_include_directories(${tgt} INTERFACE "${CMAKE_CURRENT_LIST_DIR}")
