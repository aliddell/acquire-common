#ifndef H_ACQUIRE_DRIVER_V0
#define H_ACQUIRE_DRIVER_V0

#include "device/kit/driver.h"

#ifdef __cplusplus
extern "C"
{
#endif

    enum DeviceStatusCode driver_open_device(struct Driver* self,
                                             uint8_t device_id,
                                             struct Device** out);

    enum DeviceStatusCode driver_close_device(struct Device* device);

#ifdef __cplusplus
}
#endif

#endif // H_ACQUIRE_DRIVER_V0
