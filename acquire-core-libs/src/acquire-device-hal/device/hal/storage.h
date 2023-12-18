#ifndef H_ACQUIRE_HAL_STORAGE_V0
#define H_ACQUIRE_HAL_STORAGE_V0

#include "device/kit/storage.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /// Checks that a storage device can be initialize with the given
    /// properties.
    /// @returns True if properties appear valid, otherwise False
    int storage_validate(const struct DeviceManager* system,
                         const struct DeviceIdentifier* identifier,
                         const struct StorageProperties* settings);

    /// @brief Open a storage device identified by @p identifier.
    /// @param[in] system The device manager.
    /// @return A pointer to the storage device, or NULL if the device could not
    /// be opened.
    struct Storage* storage_open(const struct DeviceManager* system,
                                 const struct DeviceIdentifier* identifier);

    /// @brief Set the storage device properties.
    /// @param[in] settings The properties to set.
    /// @returns Device_Ok if the properties were set successfully, otherwise
    /// Device_Err.
    enum DeviceStatusCode storage_set(struct Storage* self,
                                      const struct StorageProperties* settings);

    enum DeviceStatusCode storage_get(const struct Storage* self,
                                      struct StorageProperties* settings);

    enum DeviceStatusCode storage_get_meta(
      const struct Storage* self,
      struct StoragePropertyMetadata* meta);

    /// @brief Start the storage device.
    /// @details This function signals the storage device that data is about to
    /// begin streaming to the storage. Any preparation the device needs to do

    /// should be handled here (e.g. opening a file or a network connection).
    /// A successful call to this function will set self->state to
    /// DeviceState_Running.
    /// @returns Device_Ok if the device was started successfully, otherwise
    /// Device_Err.
    enum DeviceStatusCode storage_start(struct Storage* storage);

    /// @brief Stop the storage device.
    /// @details This function signals the storage device that no more data is
    /// forthcoming. The device is free to close any open files or network
    /// connections. A successful call to this function will set self->state to
    /// DeviceState_Armed.
    /// @returns Device_Ok if the device was stopped successfully, otherwise
    /// Device_Err.
    enum DeviceStatusCode storage_stop(struct Storage* storage);

    /// @brief Append data in `[beg,end)` to Storage
    /// @param[in] beg The beginning of the packet of frames to write.
    /// @param[in] end The end of the packet of frames to write.
    enum DeviceStatusCode storage_append(struct Storage* self,
                                         const struct VideoFrame* beg,
                                         const struct VideoFrame* end);

    /// @brief Close the storage device.
    /// @details The storage device is deallocated and any resources it was
    /// using are freed.
    void storage_close(struct Storage* self);

    enum DeviceState storage_get_state(const struct Storage* self);

    /// @brief Alert the storage device to expect a particular image shape.
    /// @param shape [in] The image shape to expect.
    enum DeviceStatusCode storage_reserve_image_shape(
      struct Storage* self,
      const struct ImageShape* shape);

#ifdef __cplusplus
}
#endif

#endif // H_ACQUIRE_HAL_STORAGE_V0
