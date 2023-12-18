#ifndef ACQUIRE_DEVICE_KIT_DRIVER_V0
#define ACQUIRE_DEVICE_KIT_DRIVER_V0

#include "device/props/device.h"

#ifdef _WIN32
#define acquire_export __declspec(dllexport)
#else
#define acquire_export
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    struct Driver
    {
        uint32_t (*device_count)(struct Driver*);

        enum DeviceStatusCode (*describe)(const struct Driver* self,
                                          struct DeviceIdentifier* identifier,
                                          uint64_t i);

        /// Opens a device, preparing it to be in the `AwaitingConfiguration`
        /// state.
        ///
        /// This function is normally called by `driver_open_device()` which in
        /// turn is called by functions like `camera_open()` or `storage_open()`
        /// in the hardware abstraction layer (HAL). `driver_open_device()` is
        /// responsible for actually populating the fields of the `Device`
        /// struct.
        ///
        /// # Example
        ///
        /// It doesn't do anything but a minimal implementation might look like:
        ///
        /// ```
        /// enum DeviceStatusCode (*open)(struct Driver* self,
        ///                                      uint64_t device_id,
        ///                                      struct Device** out)
        /// {
        ///     *out=(struct Device*)malloc(sizeof(struct Device));
        /// }
        /// ```
        ///
        /// @param[in] self The `Driver` instance.
        /// @param[in] device_id An identifier telling the `Driver` which device
        ///                      to open.
        /// @param[out] out Used to return a `Device*` as a handle to the opened
        ///                 device.
        enum DeviceStatusCode (*open)(struct Driver* self,
                                      uint64_t device_id,
                                      struct Device** out);

        /// Closes a device and releases any acquired resources.
        enum DeviceStatusCode (*close)(struct Driver* self, struct Device* in);

        // Must be idempotent. May be called multiple times.
        // Responsible for cleaning up all driver related resources.
        enum DeviceStatusCode (*shutdown)(struct Driver* self);
    };

    acquire_export struct Driver* acquire_driver_init_v0(
      void (*reporter)(int is_error,
                       const char* file,
                       int line,
                       const char* function,
                       const char* msg));

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ACQUIRE_DEVICE_KIT_DRIVER_V0
