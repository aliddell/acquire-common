#ifndef H_ACQUIRE_KIT_STORAGE_V0
#define H_ACQUIRE_KIT_STORAGE_V0

#include "device/props/device.h"
#include "device/props/storage.h"

#ifdef __cplusplus
extern "C"
{
#endif
    struct StorageProperties;
    struct VideoFrame;

    struct Storage
    {
        struct Device device;
        enum DeviceState state;

        enum DeviceState (*set)(struct Storage* self,
                                const struct StorageProperties* settings);
        void (*get)(const struct Storage* self,
                    struct StorageProperties* settings);
        void (*get_meta)(const struct Storage* self,
                         struct StoragePropertyMetadata* metadata);
        enum DeviceState (*start)(struct Storage* self);

        /// @brief Append data in [frame,frame+*nbytes) to Storage
        /// @param frame  [in] The beginning of the packet of frames to write.
        /// @param nbytes [in,out] The number of bytes in the packet to write.
        ///                        The Storage device can consume 0 to *nbytes
        ///                        from the packet, and must set *nbytes to be
        ///                        the number of consumed bytes.
        enum DeviceState (*append)(struct Storage* self,
                                   const struct VideoFrame* frame,
                                   size_t* nbytes);
        enum DeviceState (*stop)(struct Storage* self);

        // Only call this from within storage.driver.c.
        // Should really be private to that module.
        void (*destroy)(struct Storage* self);

        /// @brief Alert the storage device to expect a particular image shape.
        /// @param shape [in] The image shape to expect.
        void (*reserve_image_shape)(struct Storage* self,
                                    const struct ImageShape* shape);
    };

#ifdef __cplusplus
}
#endif

#endif // H_ACQUIRE_KIT_STORAGE_V0
