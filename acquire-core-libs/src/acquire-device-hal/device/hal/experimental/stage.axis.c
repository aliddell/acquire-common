#include "stage.axis.h"
#include "logger.h"
#include "device/hal/driver.h"
#include "device/hal/device.manager.h"

#define containerof(P, T, F) ((T*)(((char*)(P)) - offsetof(T, F)))

#define LOG(...) aq_logger(0, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOGE(...) aq_logger(1, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define CHECK(e)                                                               \
    do {                                                                       \
        if (!(e)) {                                                            \
            LOGE("Expression evaluated as false:\n\t%s", #e);                  \
            goto Error;                                                        \
        }                                                                      \
    } while (0)
#define CHECK_NOJUMP(e)                                                        \
    do {                                                                       \
        if (!(e)) {                                                            \
            LOGE("Expression evaluated as false:\n\t%s", #e);                  \
        }                                                                      \
    } while (0)

struct StageAxis*
stage_axis_open(const struct DeviceManager* system,
                const struct DeviceIdentifier* identifier)
{
    struct StageAxis* self = 0;

    CHECK(identifier);
    CHECK(identifier->kind == DeviceKind_StageAxis);

    {
        struct Device* device = 0;
        CHECK(Device_Ok ==
              driver_open_device(device_manager_get_driver(system, identifier),
                                 identifier->device_id,
                                 &device));

        self = containerof(device, struct StageAxis, device);
    }

    // Check the required interface functions are non-null
    CHECK(self->set != NULL);
    CHECK(self->get != NULL);
    CHECK(self->get_meta != NULL);
    CHECK(self->start != NULL);
    CHECK(self->stop != NULL);

    return self;
Error:
    return 0;
}

void
stage_axis_close(struct StageAxis* self)
{
    CHECK(self);
    struct Driver* const d = self->device.driver;
    CHECK_NOJUMP(Device_Ok == d->close(d, &self->device));
    self->state = DeviceState_Closed;
Error:;
}

enum DeviceStatusCode
stage_axis_set(struct StageAxis* self, struct StageAxisProperties* settings)
{
    enum DeviceStatusCode ecode;
    // Neither can be NULL
    CHECK(self);
    CHECK(settings);
    switch (ecode = self->set(self, settings)) {
        case Device_Ok:
            if (self->state != DeviceState_Running)
                self->state = DeviceState_Armed;
            break;
        case Device_Err:
            stage_axis_stop(self);
            self->state = DeviceState_AwaitingConfiguration;
            break;
    }
    return ecode;
Error:
    return Device_Err;
}

enum DeviceStatusCode
stage_axis_get(const struct StageAxis* self,
               struct StageAxisProperties* settings)
{
    // Neither can be NULL
    CHECK(self);
    CHECK(settings);
    return self->get(self, settings);
Error:
    return Device_Err;
}

enum DeviceStatusCode
stage_axis_get_meta(const struct StageAxis* self,
                    struct StageAxisPropertyMetadata* meta)
{
    // Neither can be NULL
    CHECK(self);
    CHECK(meta);
    return self->get_meta(self, meta);
Error:
    return Device_Err;
}

enum DeviceStatusCode
stage_axis_start(struct StageAxis* self)
{
    enum DeviceStatusCode ecode;
    CHECK(self);
    switch (ecode = self->start(self)) {
        case Device_Ok:
            self->state = DeviceState_Running;
            break;
        case Device_Err:
            self->state = DeviceState_AwaitingConfiguration;
            break;
    }
    return ecode;
Error:
    return Device_Err;
}

enum DeviceStatusCode
stage_axis_stop(struct StageAxis* self)
{
    enum DeviceStatusCode ecode = Device_Ok;
    CHECK(self);
    if (self->state == DeviceState_Running) {
        LOG("STAGE AXIS STOP %s", self->device.identifier.name);
        switch (ecode = self->stop(self)) {
            case Device_Ok:
                self->state = DeviceState_Armed;
                break;
            case Device_Err:
                self->state = DeviceState_AwaitingConfiguration;
                break;
        }
    }
Finalize:
    return ecode;
Error:
    ecode = Device_Err;
    goto Finalize;
}
