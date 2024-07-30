/// @file simcam-will-not-stall.cpp
/// Test that we can acquire frames from a slow-moving camera without hanging.

#include "acquire.h"
#include "device/hal/device.manager.h"
#include "platform.h"
#include "logger.h"

#include <cstdio>
#include <regex>
#include <stdexcept>
#include <string>

namespace {
class IntrospectiveLogger
{
  public:
    IntrospectiveLogger()
      : dropped_frames(0)
      , re("Dropped\\s*(\\d+)"){};

    // inspect for "[stream 0] Dropped", otherwise pass the message through
    void report_and_inspect(int is_error,
                            const char* file,
                            int line,
                            const char* function,
                            const char* msg)
    {
        std::string m(msg);

        std::smatch matches;
        if (std::regex_search(m, matches, re)) {
            dropped_frames += std::stoi(matches[1]);
        }

        printf("%s%s(%d) - %s: %s\n",
               is_error ? "ERROR " : "",
               file,
               line,
               function,
               msg);
    }

    size_t get_dropped_frames() const { return dropped_frames; }
    void reset() { dropped_frames = 0; }

  private:
    size_t dropped_frames;
    std::regex re;
} introspective_logger;
} // namespace

static void
reporter(int is_error,
         const char* file,
         int line,
         const char* function,
         const char* msg)
{
    introspective_logger.report_and_inspect(
      is_error, file, line, function, msg);
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

const static size_t frame_count = 100;

void
configure(AcquireRuntime* runtime, std::string_view camera_type)
{
    CHECK(runtime);

    const DeviceManager* dm = acquire_device_manager(runtime);
    CHECK(dm);

    AcquireProperties props = {};
    OK(acquire_get_configuration(runtime, &props));

    DEVOK(device_manager_select(dm,
                                DeviceKind_Camera,
                                camera_type.data(),
                                camera_type.size(),
                                &props.video[0].camera.identifier));
    DEVOK(device_manager_select(dm,
                                DeviceKind_Storage,
                                SIZED("trash") - 1,
                                &props.video[0].storage.identifier));

    OK(acquire_configure(runtime, &props));

    props.video[0].camera.settings.binning = 1;
    props.video[0].camera.settings.pixel_type = SampleType_u16;
    props.video[0].camera.settings.shape = {
        .x = 1920,
        .y = 1080,
    };
    props.video[0].camera.settings.exposure_time_us = 1; // very small exposure

    props.video[0].max_frame_count = frame_count;

    OK(acquire_configure(runtime, &props));
}

void
acquire(AcquireRuntime* runtime)
{
    const auto next = [](VideoFrame* cur) -> VideoFrame* {
        return (VideoFrame*)(((uint8_t*)cur) + cur->bytes_of_frame);
    };

    const auto consumed_bytes = [](const VideoFrame* const cur,
                                   const VideoFrame* const end) -> size_t {
        return (uint8_t*)end - (uint8_t*)cur;
    };

    AcquireProperties props = {};
    OK(acquire_get_configuration(runtime, &props));

    // expected time to acquire frames + 100%
    static double time_limit_ms =
      (props.video[0].max_frame_count / 3.0) * 1000.0 * 2.0;

    struct clock clock = {};
    clock_init(&clock);
    clock_shift_ms(&clock, time_limit_ms);

    OK(acquire_start(runtime));
    {
        uint64_t nframes = 0;
        while (nframes < props.video[0].max_frame_count) {
            struct clock throttle = {};
            clock_init(&throttle);

            EXPECT(clock_cmp_now(&clock) < 0,
                   "Timeout at %f ms",
                   clock_toc_ms(&clock) + time_limit_ms);

            VideoFrame *beg, *end, *cur;

            OK(acquire_map_read(runtime, 0, &beg, &end));
            for (cur = beg; cur < end; cur = next(cur)) {
                ++nframes;
            }

            uint32_t n = (uint32_t)consumed_bytes(beg, end);
            OK(acquire_unmap_read(runtime, 0, n));

            clock_sleep_ms(&throttle, 100.0f);
        }

        CHECK(nframes == props.video[0].max_frame_count);
    }

    OK(acquire_stop(runtime));
}

int
main()
{
    int retval = 1;

    AcquireRuntime* runtime = acquire_init(reporter);

    try {
        configure(runtime, "simulated.*sin*");
        acquire(runtime);
        CHECK(introspective_logger.get_dropped_frames() < frame_count);

        introspective_logger.reset();

        configure(runtime, "simulated.*empty.*");
        acquire(runtime);
        CHECK(introspective_logger.get_dropped_frames() >= frame_count);

        retval = 0;
    } catch (const std::exception& e) {
        ERR("Exception: %s", e.what());
    }

    acquire_shutdown(runtime);

    return retval;
}
