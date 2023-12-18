#ifndef H_ACQUIRE_PROPS_STAGE_AXIS_V0
#define H_ACQUIRE_PROPS_STAGE_AXIS_V0

#include "../components.h"
#include "../metadata.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct DeviceManager;

    struct StageAxisProperties
    {
        struct stage_axis_properties_state_s
        {
            float position;
            float velocity;
        } target, immediate;
        struct PID feedback;
    };

    struct StageAxisPropertyMetadata
    {
        struct Property position;
        struct Property velocity;
        struct
        {
            struct Property proportional;
            struct Property integral;
            struct Property derivative;
        } PID;
    };

#ifdef __cplusplus
}
#endif

#endif // H_ACQUIRE_PROPS_STAGE_AXIS_V0
