#include "loader.h"
#include "platform.h"
#include "logger.h"

#include <stdlib.h>

#define containerof(ptr, T, V) ((T*)(((char*)(ptr)) - offsetof(T, V)))

#define L (aq_logger)
#define LOG(...) L(0, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define ERR(...) L(1, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define EXPECT(e, ...)                                                         \
    do {                                                                       \
        if (!((e))) {                                                          \
            ERR(__VA_ARGS__);                                                  \
            goto Error;                                                        \
        }                                                                      \
    } while (0)
#define CHECK(e) EXPECT(e, "Expression was false:\n\t%s\n", #e)

#define TRACE(...)
// #define TRACE(...) LOG(__VA_ARGS__)

typedef struct Driver* (*driver_init_proc_t)(
  void (*reporter)(int is_error,
                   const char* file,
                   int line,
                   const char* function,
                   const char* msg));

struct Loader
{
    struct Driver driver;
    struct Driver* inner;
    struct lib lib;
};

static unsigned
device_count(struct Driver* self_)
{
    struct Loader* self = containerof(self_, struct Loader, driver);
    if (!self->inner)
        return 0;
    return self->inner->device_count(self->inner);
}

static enum DeviceStatusCode
describe(const struct Driver* self_,
         struct DeviceIdentifier* identifier,
         uint64_t i)
{
    struct Loader* self = containerof(self_, struct Loader, driver);
    if (!self->inner)
        return Device_Err;
    return self->inner->describe(self->inner, identifier, i);
}

static enum DeviceStatusCode
open(struct Driver* self_, uint64_t device_id, struct Device** out)
{
    struct Loader* self = containerof(self_, struct Loader, driver);
    if (!self->inner)
        return Device_Err;
    return self->inner->open(self->inner, device_id, out);
}

static enum DeviceStatusCode
close(struct Driver* self_, struct Device* in)
{
    struct Loader* self = containerof(self_, struct Loader, driver);
    if (!self->inner)
        return Device_Err;
    return self->inner->close(self->inner, in);
}

static enum DeviceStatusCode
shutdown_(struct Driver* self_)
{
    struct Loader* self = containerof(self_, struct Loader, driver);
    enum DeviceStatusCode ecode = Device_Err;
    if (self->inner)
        ecode = self->inner->shutdown(self->inner);
    self->inner = 0;
    lib_close(&self->lib);
    free(self);
    return ecode;
}

struct Driver*
driver_load(const char* relative_path,
            void (*reporter)(int is_error,
                             const char* file,
                             int line,
                             const char* function,
                             const char* msg))
{
    struct Loader* self = malloc(sizeof(*self));
    EXPECT(self, "Failed to allocate %d bytes.", sizeof(*self));

    *self = (struct Loader){
        .driver =
          (struct Driver){
            .device_count = device_count,
            .describe = describe,
            .open = open,
            .close = close,
            .shutdown = shutdown_,
          },
    };

    TRACE("LOADER: REQUEST %s", relative_path);
    EXPECT(lib_open_by_name(&self->lib, relative_path),
           "Failed to load driver at \"%s\".",
           relative_path);

    driver_init_proc_t init = 0;
    const char* const entry_point = "acquire_driver_init_v0";
    EXPECT(init = lib_load(&self->lib, entry_point),
           "Entry point not found for driver. Missing \"%s\" in \"%s\"",
           entry_point,
           relative_path);

    EXPECT(self->inner = init(reporter),
           "Failed to initialize driver at \"%s\"",
           relative_path);

    return &self->driver;
Error:
    if (self) {
        lib_close(&self->lib);
        free(self);
    }
    return 0;
}
