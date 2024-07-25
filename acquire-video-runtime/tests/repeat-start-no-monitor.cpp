/// @file repeat-start-no-monitor.cpp
/// Test that we can repeatedly acquire without unwittingly initializing the
/// monitor reader.

#include "acquire.h"
#include "device/hal/device.manager.h"
#include "platform.h"
#include "logger.h"

#include <cstdio>
#include <stdexcept>

void
reporter(int is_error,
         const char* file,
         int line,
         const char* function,
         const char* msg)
{
    fprintf(is_error ? stderr : stdout,
            "%s%s(%d) - %s: %s\n",
            is_error ? "ERROR " : "",
            file,
            line,
            function,
            msg);
}

/// Helper for passing size static strings as function args.
/// For a function: `f(char*,size_t)` use `f(SIZED("hello"))`.
/// Expands to `f("hello",5)`.
#define SIZED(str) str, sizeof(str)

#define L (aq_logger)
#define LOG(...) L(0, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define ERR(...) L(1, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define EXPECT(e, ...)                                                         \
    do {                                                                       \
        if (!(e)) {                                                            \
            char buf[1 << 8] = { 0 };                                          \
            ERR(__VA_ARGS__);                                                  \
            snprintf(buf, sizeof(buf) - 1, __VA_ARGS__);                       \
            throw std::runtime_error(buf);                                     \
        }                                                                      \
    } while (0)
#define CHECK(e) EXPECT(e, "Expression evaluated as false: %s", #e)
#define DEVOK(e) CHECK(Device_Ok == (e))
#define OK(e) CHECK(AcquireStatus_Ok == (e))

void
configure(AcquireRuntime* runtime)
{
    CHECK(runtime);

    const DeviceManager* dm = acquire_device_manager(runtime);
    CHECK(dm);

    AcquireProperties props = {};
    OK(acquire_get_configuration(runtime, &props));

    // configure camera
    DEVOK(device_manager_select(dm,
                                DeviceKind_Camera,
                                SIZED("simulated.*empty.*") - 1,
                                &props.video[0].camera.identifier));

    props.video[0].camera.settings.binning = 1;
    props.video[0].camera.settings.pixel_type = SampleType_u16;
    props.video[0].camera.settings.shape = {
        .x = 2304,
        .y = 2304,
    };

    // configure acquisition
    props.video[0].max_frame_count = 500;

    // configure storage
    DEVOK(device_manager_select(dm,
                                DeviceKind_Storage,
                                SIZED("trash") - 1,
                                &props.video[0].storage.identifier));
    storage_properties_init(
      &props.video[0].storage.settings, 0, nullptr, 0, nullptr, 0, { 0 }, 0);

    OK(acquire_configure(runtime, &props));
}

void
acquire(AcquireRuntime* runtime)
{
    CHECK(runtime);

    OK(acquire_start(runtime));
    OK(acquire_stop(runtime));
}

int
main()
{
    AcquireRuntime* runtime = acquire_init(reporter);

    try {
        for (auto i = 0; i < 2; ++i) {
            configure(runtime);
            acquire(runtime);
        }
    } catch (const std::exception& e) {
        ERR("Caught exception: %s", e.what());
        acquire_shutdown(runtime);
        return 1;
    }

    OK(acquire_shutdown(runtime));
    return 0;
}
