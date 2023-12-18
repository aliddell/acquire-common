#include "driver.h"
#include "platform.h"
#include "logger.h"

#define LOG(...) aq_logger(0, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOGE(...) aq_logger(1, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define EXPECT(e, ...)                                                         \
    do {                                                                       \
        if (!(e)) {                                                            \
            LOGE(__VA_ARGS__);                                                 \
            goto Error;                                                        \
        }                                                                      \
    } while (0)
#define CHECK(e) EXPECT(e, "Expression evaluated as false:\n\t%s", #e)
#define CHECK_NOJUMP(e)                                                        \
    do {                                                                       \
        if (!(e)) {                                                            \
            LOGE("Expression evaluated as false:\n\t%s", #e);                  \
            goto Error;                                                        \
        }                                                                      \
    } while (0)

enum DeviceStatusCode
driver_open_device(struct Driver* driver,
                   uint8_t device_id,
                   struct Device** out)
{
    EXPECT(driver, "Invalid parameter. `driver` was NULL.");
    EXPECT(driver->open, "`driver` has a NULL `open()` function.");
    CHECK(Device_Ok == driver->open(driver, device_id, out));

    CHECK(*out);
    CHECK(Device_Ok ==
          driver->describe(driver, &out[0]->identifier, device_id));
    (*out)->driver = driver;
    return Device_Ok;
Error:
    return Device_Err;
}

enum DeviceStatusCode
driver_close_device(struct Device* device)
{
    struct Driver* const driver = device->driver;
    CHECK_NOJUMP(Device_Ok == driver->close(driver, device));
    return Device_Ok;
Error:
    return Device_Err;
}
