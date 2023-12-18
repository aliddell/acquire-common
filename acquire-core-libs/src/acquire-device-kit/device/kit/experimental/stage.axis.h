#ifndef H_ACQUIRE_KIT_STAGE_AXIS_V0
#define H_ACQUIRE_KIT_STAGE_AXIS_V0

#include "device/props/experimental/stage.axis.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct StageAxis
    {
        struct Device device;
        enum DeviceState state;

        enum DeviceStatusCode (*set)(struct StageAxis*,
                                     struct StageAxisProperties* settings);
        enum DeviceStatusCode (*get)(const struct StageAxis*,
                                     struct StageAxisProperties* settings);
        enum DeviceStatusCode (*get_meta)(
          const struct StageAxis*,
          struct StageAxisPropertyMetadata* meta);
        enum DeviceStatusCode (*start)(struct StageAxis*);
        enum DeviceStatusCode (*stop)(struct StageAxis*);
    };

#ifdef __cplusplus
}
#endif

#endif // H_ACQUIRE_KIT_STAGE_AXIS_V0
