#ifndef H_ACQUIRE_KIT_SIGNALS_V0
#define H_ACQUIRE_KIT_SIGNALS_V0

#include "device/props/device.h"
#include "device/props/experimental/signals.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct SignalProperties;
    struct SignalPropertyMetadata;

    struct Signal
    {
        struct Device device;
        enum DeviceState state;

        enum DeviceState (*set)(struct Signal* self,
                                struct SignalProperties* settings);
        enum DeviceState (*get)(const struct Signal* self,
                                struct SignalProperties* settings);
        enum DeviceState (*get_meta)(const struct Signal* self,
                                     struct SignalPropertyMetadata* meta);
        enum DeviceState (*start)(struct Signal* self);
        enum DeviceState (*stop)(struct Signal* self);

        enum DeviceState (*write_ao)(struct Signal* self,
                                     uint8_t* buf,
                                     size_t nbytes);
        // TODO: Finish Signal.
    };

#ifdef __cplusplus
};
#endif

#endif // H_ACQUIRE_KIT_SIGNALS_V0
