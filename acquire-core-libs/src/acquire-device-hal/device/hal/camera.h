#ifndef H_ACQUIRE_HAL_CAMERA_V0
#define H_ACQUIRE_HAL_CAMERA_V0

#include "device/kit/camera.h"
#include "device.manager.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct Camera* camera_open(const struct DeviceManager* system,
                               const struct DeviceIdentifier* identifier);

    void camera_close(struct Camera* camera);

    enum DeviceStatusCode camera_set(struct Camera* camera,
                                     struct CameraProperties* settings);

    enum DeviceStatusCode camera_get(const struct Camera* camera,
                                     struct CameraProperties* settings);

    enum DeviceStatusCode camera_get_meta(const struct Camera* camera,
                                          struct CameraPropertyMetadata* meta);

    enum DeviceStatusCode camera_get_image_shape(const struct Camera* camera,
                                                 struct ImageShape* shape);

    enum DeviceStatusCode camera_start(struct Camera* camera);

    enum DeviceStatusCode camera_stop(struct Camera* camera);

    enum DeviceStatusCode camera_execute_trigger(struct Camera* camera);

    enum DeviceStatusCode camera_get_frame(struct Camera* camera,
                                           void* im,
                                           size_t* nbytes,
                                           struct ImageInfo* info);

    enum DeviceState camera_get_state(const struct Camera* camera);

#ifdef __cplusplus
}
#endif

#endif // H_ACQUIRE_HAL_CAMERA_V0
