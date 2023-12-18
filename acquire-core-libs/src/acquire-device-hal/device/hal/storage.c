#include "storage.h"
#include "logger.h"
#include "device.manager.h"
#include "driver.h"

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#define containerof(P, T, F) ((T*)(((char*)(P)) - offsetof(T, F)))
#define countof(e) (sizeof(e) / sizeof((e)[0]))

#define LOG(...) aq_logger(0, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOGE(...) aq_logger(1, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define EXPECT(e, ...)                                                         \
    do {                                                                       \
        if (!(e)) {                                                            \
            LOGE(__VA_ARGS__);                                                 \
            goto Error;                                                        \
        }                                                                      \
    } while (0)
#define CHECK(e) EXPECT(e, #e)
#define CHECK_NOJUMP(e)                                                        \
    do {                                                                       \
        if (!(e)) {                                                            \
            LOGE("Expression evaluated as false:\n\t%s", #e);                  \
        }                                                                      \
    } while (0)
#define CHECK_SILENT(e)                                                        \
    do {                                                                       \
        if (!(e)) {                                                            \
            goto Error;                                                        \
        }                                                                      \
    } while (0)

//
//                  STORAGE
//

int
storage_validate(const struct DeviceManager* system,
                 const struct DeviceIdentifier* identifier,
                 const struct StorageProperties* settings)
{
    int is_ok = 1;
    struct Storage* self = 0;
    {
        struct Device* device = 0;
        EXPECT(identifier->kind == DeviceKind_Storage,
               "Expected a Storage device. Got %s.",
               device_kind_as_string(identifier->kind));
        CHECK(Device_Ok ==
              driver_open_device(device_manager_get_driver(system, identifier),
                                 identifier->device_id,
                                 &device));
        EXPECT(device->identifier.kind == DeviceKind_Storage,
               "Expected a Storage device, but a %s device was opened.",
               device_kind_as_string(device->identifier.kind));
        device->identifier = *identifier;
        self = containerof(device, struct Storage, device);
    }
    if (self) {
        self->state = self->set(self, settings);
        CHECK(self->state == DeviceState_Armed);
    }
Finalize:
    storage_close(self);
    return is_ok;
Error:
    is_ok = 0;
    goto Finalize;
}

struct Storage*
storage_open(const struct DeviceManager* system,
             const struct DeviceIdentifier* identifier)
{
    struct Storage* self = 0;

    CHECK(identifier);
    CHECK(identifier->kind == DeviceKind_Storage);

    {
        struct Device* device = 0;
        CHECK(Device_Ok ==
              driver_open_device(device_manager_get_driver(system, identifier),
                                 identifier->device_id,
                                 &device));
        self = containerof(device, struct Storage, device);
    }

    // Check the required interface functions are non-null
    CHECK(self->set != NULL);
    CHECK(self->get != NULL);
    CHECK(self->get_meta != NULL);
    CHECK(self->start != NULL);
    CHECK(self->append != NULL);
    CHECK(self->stop != NULL);
    CHECK(self->destroy != NULL);
    CHECK(self->reserve_image_shape != NULL);

    return self;
Error:
    storage_close(self);
    return 0;
}

enum DeviceStatusCode
storage_set(struct Storage* self, const struct StorageProperties* settings)
{
    CHECK(self);
    CHECK(settings);

    self->state = self->set(self, settings);
    EXPECT(DeviceState_Armed == self->state,
           "Expected Armed. Got %s.",
           device_state_as_string(self->state));

    return Device_Ok;

Error:
    return Device_Err;
}

enum DeviceStatusCode
storage_get(const struct Storage* self, struct StorageProperties* settings)
{
    CHECK(self);
    CHECK(self->get);
    self->get(self, settings);
    return Device_Ok;
Error:
    return Device_Err;
}

enum DeviceStatusCode
storage_get_meta(const struct Storage* self,
                 struct StoragePropertyMetadata* meta)
{
    CHECK(self);
    CHECK(self->get_meta);
    self->get_meta(self, meta);
    return Device_Ok;
Error:
    return Device_Err;
}

enum DeviceStatusCode
storage_start(struct Storage* self)
{
    CHECK(self);
    CHECK(self->state == DeviceState_Armed);

    enum DeviceStatusCode status_code;
    switch (self->state = self->start(self)) {
        case DeviceState_Running:
            status_code = Device_Ok;
            break;
        default:
            status_code = Device_Err;
            break;
    }

    return status_code;
Error:
    return Device_Err;
}

enum DeviceStatusCode
storage_stop(struct Storage* self)
{
    enum DeviceStatusCode ecode = Device_Ok;
    CHECK(self);
    CHECK(self->stop);
    if (self->state == DeviceState_Running) {
        EXPECT((self->state = self->stop(self)) == DeviceState_Armed ||
                 self->state == DeviceState_AwaitingConfiguration,
               "Expected Armed or AwaitingConfiguration. Got state: %s.",
               device_state_as_string(self->state));
    }

Finalize:
    return ecode;
Error:
    ecode = Device_Err;
    goto Finalize;
}

enum DeviceStatusCode
storage_append(struct Storage* self,
               const struct VideoFrame* beg,
               const struct VideoFrame* end)
{
    CHECK(self);
    CHECK(self->state == DeviceState_Running);
    CHECK(end >= beg);
    if (beg < end) {
        size_t nbytes = (uint8_t*)end - (uint8_t*)beg;
        // FIXME: (nclack) api inconsistency. What happens if we don't consume
        // all bytes?
        self->state = self->append(self, beg, &nbytes);
        CHECK(self->state == DeviceState_Running);
    }
    return Device_Ok;
Error:
    return Device_Err;
}

void
storage_close(struct Storage* self)
{
    CHECK(self);
    storage_stop(self);

    driver_close_device(&self->device);
    self->state = DeviceState_Closed;
Error:;
}

enum DeviceState
storage_get_state(const struct Storage* const self)
{
    CHECK(self);
    return self->state;
Error:
    return DeviceState_Closed;
}

enum DeviceStatusCode
storage_reserve_image_shape(struct Storage* self,
                            const struct ImageShape* shape)
{
    CHECK(self);
    CHECK(self->reserve_image_shape);
    self->reserve_image_shape(self, shape);
    return Device_Ok;
Error:
    return Device_Err;
}
