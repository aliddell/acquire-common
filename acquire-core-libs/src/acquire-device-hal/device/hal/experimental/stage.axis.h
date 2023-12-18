#ifndef H_ACQUIRE_HAL_STAGE_AXIS_V0
#define H_ACQUIRE_HAL_STAGE_AXIS_V0

#include "device/hal/device.manager.h"
#include "device/kit/experimental/stage.axis.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct StageAxis* stage_axis_open(
      const struct DeviceManager* system,
      const struct DeviceIdentifier* identifier);

    void stage_axis_close(struct StageAxis* self);

    enum DeviceStatusCode stage_axis_set(struct StageAxis* self,
                                         struct StageAxisProperties* settings);

    enum DeviceStatusCode stage_axis_get(const struct StageAxis* self,
                                         struct StageAxisProperties* settings);

    enum DeviceStatusCode stage_axis_get_meta(
      const struct StageAxis* self,
      struct StageAxisPropertyMetadata* meta);

    enum DeviceStatusCode stage_axis_start(struct StageAxis* self);

    enum DeviceStatusCode stage_axis_stop(struct StageAxis* self);

#ifdef __cplusplus
}
#endif

#endif // H_ACQUIRE_HAL_STAGE_AXIS_V0
