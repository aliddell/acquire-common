#ifndef H_ACQUIRE_PROPS_DEVICE_V0
#define H_ACQUIRE_PROPS_DEVICE_V0

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    enum DeviceStatusCode
    {
        Device_Ok,
        Device_Err
    };

    enum DeviceState
    {
        DeviceState_Closed,
        DeviceState_AwaitingConfiguration,
        DeviceState_Armed,
        DeviceState_Running,
        DeviceStateCount,
    };

    // NOTE: is you update this, be sure to update device_kind_as_string()!
    enum DeviceKind
    {
        DeviceKind_None,
        DeviceKind_Camera,
        DeviceKind_Storage,
        DeviceKind_StageAxis,
        DeviceKind_Signals,
        DeviceKind_Count,
        DeviceKind_Unknown
    };

    struct DeviceIdentifier
    {
        uint8_t driver_id; // device manager populates this
        uint8_t device_id;
        enum DeviceKind kind;
        char name[256];
    };

    /// Marker type identifying that an object is a "Device".
    struct Device
    {
        struct DeviceIdentifier identifier;

        /// Set by `device_open_device()`
        struct Driver* driver;
    };

    size_t device_identifier_as_debug_string(
      char* buf,
      size_t nbytes,
      const struct DeviceIdentifier* identifier);

    const char* device_kind_as_string(enum DeviceKind state);
    const char* device_state_as_string(enum DeviceState state);

#ifdef __cplusplus
}

#endif

#endif // H_ACQUIRE_PROPS_DEVICE_V0
