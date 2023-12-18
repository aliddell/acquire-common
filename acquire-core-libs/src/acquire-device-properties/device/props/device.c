#include <stdio.h>
#include "device.h"

#define countof(e) (sizeof(e) / sizeof(*(e)))

size_t
device_identifier_as_debug_string(char* buf,
                                  size_t nbytes,
                                  const struct DeviceIdentifier* identifier)
{
    return snprintf(buf, // NOLINT
                    nbytes,
                    "(%s) %s",
                    device_kind_as_string(identifier->kind),
                    identifier->name);
}

const char*
device_kind_as_string(enum DeviceKind kind)
{
    // Note: This table needs to get updated whenever a DeviceKind gets
    //       added. The unit test below should crash when an entry is
    //       missing.

    // clang-format off
    const char* table[] = {
#define XXX(name) [DeviceKind_##name] = #name
        XXX(None),
        XXX(Camera),
        XXX(Storage),
        XXX(StageAxis),
        XXX(Signals),
#undef XXX
    };
    // clang-format on
    if (kind >= countof(table))
        return "(unknown)";

    return table[kind];
}

const char*
device_state_as_string(enum DeviceState state)
{
    // Note: This table needs to get updated whenever a DeviceState gets
    //       added. The unit test below should crash when an entry is
    //       missing.

    // clang-format off
    const char* table[] = {
#define XXX(name) [DeviceState_##name] = #name
        XXX(Closed),
        XXX(AwaitingConfiguration),
        XXX(Armed),
        XXX(Running),
#undef XXX
    };
    // clang-format on
    if (state >= countof(table))
        return "(unknown)";

    return table[state];
}

//
//  UNIT TESTS
//

#ifndef NO_UNIT_TESTS
#include "logger.h"

#define L (aq_logger)
#define LOG(...) L(0, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define ERR(...) L(1, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define CHECK(e)                                                               \
    do {                                                                       \
        if (!(e)) {                                                            \
            ERR("Expression evaluated as false:\n\t%s", #e);                   \
            goto Error;                                                        \
        }                                                                      \
    } while (0)

int
unit_test__device_state_as_string__is_defined_for_all()
{
    for (int i = 0; i < DeviceStateCount; ++i) {
        CHECK(device_state_as_string(i)[0] != '(');
    }
    return 1;
Error:
    return 0;
}

int
unit_test__device_kind_as_string__is_defined_for_all()
{
    for (int i = 0; i < DeviceKind_Count; ++i) {
        // Check this isn't returning "unknown" for known counts
        CHECK(device_kind_as_string(i)[0] != '(');
    }
    return 1;
Error:
    return 0;
}
#endif
