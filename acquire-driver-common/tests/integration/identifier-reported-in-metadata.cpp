/// @file identifier-reported-in-metadata.cpp
/// @brief Test that the device identifier name is reported in an acquisition
/// with simulated cameras and common storage.

#include "acquire.h"
#include "device/hal/device.manager.h"
#include "logger.h"

#include <cstdio>
#include <stdexcept>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

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
#define SIZED(str) str.c_str(), str.size()

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

/// Check that strings a == b
/// example: `ASSERT_STREQ("foo",container_of_foo)`
#define ASSERT_STREQ(a, b)                                                     \
    do {                                                                       \
        EXPECT(a == b,                                                         \
               "Expected '%s'=='%s' but '%s'!= '%s'",                          \
               #a,                                                             \
               #b,                                                             \
               a.c_str(),                                                      \
               b.c_str());                                                     \
    } while (0)

void
check_name_reported(AcquireRuntime* runtime,
                    const std::string& camera,
                    const std::string& storage)
{
    CHECK(runtime);

    AcquireProperties props = {};
    OK(acquire_get_configuration(runtime, &props));

    const DeviceManager* dm = acquire_device_manager(runtime);
    CHECK(dm);

    DEVOK(device_manager_select(
      dm, DeviceKind_Camera, SIZED(camera), &props.video[0].camera.identifier));
    DEVOK(device_manager_select(dm,
                                DeviceKind_Storage,
                                SIZED(storage),
                                &props.video[0].storage.identifier));

    storage_properties_init(&props.video[0].storage.settings,
                            0,
                            "out",
                            strlen("out") + 1,
                            "{'hello':'world'}",
                            strlen("{'hello':'world'}") + 1,
                            { 0 },
                            0);
    OK(acquire_configure(runtime, &props));

    AcquirePropertyMetadata metadata = { 0 };
    OK(acquire_get_configuration_metadata(runtime, &metadata));

    std::string camera_reported{ metadata.video[0].camera.name.str,
                                 metadata.video[0].camera.name.nbytes - 1 };
    ASSERT_STREQ(camera, camera_reported);

    std::string storage_reported{ metadata.video[0].storage.name.str,
                                  metadata.video[0].storage.name.nbytes - 1 };
    ASSERT_STREQ(storage, storage_reported);
}

int
main()
{
    int retval = 1;
    AcquireRuntime* runtime = acquire_init(reporter);

    std::vector<std::string> cameras{
        "simulated: uniform random",
        "simulated: radial sin",
        "simulated: empty",
    };
    std::vector<std::string> storage{
        "raw",
        "tiff",
        "trash",
        "tiff-json",
    };

    try {
        for (const auto& c : cameras) {
            for (const auto& s : storage) {
                LOG("Configuring '%s' camera with '%s' storage",
                    c.c_str(),
                    s.c_str());
                check_name_reported(runtime, c, s);
                LOG("Done (OK)");
            }
        }
        LOG("Done (OK)");

        retval = 0;
    } catch (const std::exception& e) {
        ERR("Exception: %s", e.what());
    }

    acquire_shutdown(runtime);
    return retval;
}
