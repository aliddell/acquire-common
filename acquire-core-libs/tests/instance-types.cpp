// Just check that we can instance the types.

#include "device/kit/driver.h"
#include "device/kit/storage.h"
#include "device/kit/camera.h"

#include <stdio.h>

#define EXPECT(e, ...)                                                         \
    do {                                                                       \
        if (!(e)) {                                                            \
            printf(__VA_ARGS__);                                               \
            error_code = __LINE__;                                             \
        }                                                                      \
    } while (0)

/// Check that a==b
/// example: `ASSERT_EQ(int,"%d",42,meaning_of_life())`
#define ASSERT_EQ(T, fmt, a, b)                                                \
    do {                                                                       \
        T a_ = (T)(a);                                                         \
        T b_ = (T)(b);                                                         \
        EXPECT(                                                                \
          a_ == b_, "Expected %s==%s but " fmt "!=" fmt "\n", #a, #b, a_, b_); \
    } while (0)

int
main(int n, char** args)
{
    int error_code = 0;

    // If these fail, you may need a version bump on the interface.
    ASSERT_EQ(int, "%d", sizeof(struct Driver), 40);
    ASSERT_EQ(int, "%d", sizeof(struct Camera), 344);
    ASSERT_EQ(int, "%d", sizeof(struct Storage), 344);

    return error_code;
}
