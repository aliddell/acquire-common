#ifndef H_ACQUIRE_LOADER_V0
#define H_ACQUIRE_LOADER_V0

#include "device/kit/driver.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /// @brief Load and initialize a `Driver` implementation in a shared
    /// library.
    /// @param[in] relative_path The path to the shared library relative to the
    /// calling module.
    /// @param[in] reporter This callback is used to log errors or other
    /// messages. Internally, it's passed to the `acquire_driver_init_v0`
    /// function.
    /// @return a non-zero `Driver` pointer on success, otherwise 0.
    ///
    /// The targeted library must expose a function:
    ///
    ///     struct Driver* acquire_driver_init_v0(void (*reporter)(...))
    ///
    /// which is called inside `driver_load()` in order ot initialize the
    /// targeted driver. The `reporter` function is used to log error or other
    /// messages. It's signature must be:
    ///
    ///     void (*reporter)(int is_error,
    ///                      const char* file,
    ///                      int line,
    ///                      const char* function,
    ///                      const char* msg)
    struct Driver* driver_load(const char* relative_path,
                               void (*reporter)(int is_error,
                                                const char* file,
                                                int line,
                                                const char* function,
                                                const char* msg));

#ifdef __cplusplus
} // extern "C"
#endif

#endif // H_ACQUIRE_LOADER_V0
