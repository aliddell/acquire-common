#include "components.h"
#include "logger.h"

#include <stdlib.h>
#include <string.h>

#define countof(e) (sizeof(e) / sizeof(*(e)))

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

const char*
sample_type_as_string(enum SampleType type)
{
    // Note: This table needs to get updated whenever a SampleType gets
    //       added. The unit test below should crash when an entry is
    //       missing.

    // clang-format off
    const char* table[] = {
#define XXX(name) [SampleType_##name] = #name
        XXX(u8),
        XXX(u16),
        XXX(i8),
        XXX(i16),
        XXX(f32),
        XXX(u10),
        XXX(u12),
        XXX(u14),
#undef XXX
    };
    // clang-format on
    if (type >= countof(table))
        return "(unknown)";

    return table[type];
}

size_t
bytes_of_type(enum SampleType type)
{
    size_t table[SampleTypeCount]; // = { 1, 2, 1, 2, 4, 2, 2, 2 };

    // clang-format off
#define XXX(s, b) table[(s)] = (b)
        XXX(SampleType_u8, 1);
        XXX(SampleType_u16, 2);
        XXX(SampleType_i8, 1);
        XXX(SampleType_i16, 2);
        XXX(SampleType_f32, 4);
        XXX(SampleType_u10, 2);
        XXX(SampleType_u12, 2);
        XXX(SampleType_u14, 2);
#undef XXX
    // clang-format on
    if (type >= countof(table))
        return 0;

    return table[type];
}

size_t
bytes_of_image(const struct ImageShape* const shape)
{
    return shape->strides.planes * bytes_of_type(shape->type);
}

int
copy_string(struct String* dst, const struct String* src)
{
    const struct String empty = { .is_ref = 1, .str = "", .nbytes = 1 };

    // if src string is null/empty, make an empty string
    if (!(src && src->str && src->nbytes)) {
        src = &empty;
    }

    if (!dst->str || dst->is_ref) {
        // dst string pointer refers to caller-allocated memory.
        // Allocate a new string on the heap.
        CHECK(dst->str = malloc(src->nbytes)); // NOLINT
        dst->nbytes = src->nbytes;
        dst->is_ref = 0; // mark as owned
    }

    CHECK(dst->is_ref == 0);
    if (src->nbytes > dst->nbytes) {
        char* str = realloc(dst->str, src->nbytes);
        if (!str) {
            LOGE("Failed to allocate %llu bytes for string copy.",
                 (unsigned long long)src->nbytes);
            goto Error;
        }
        dst->str = str;
    }

    dst->nbytes = src->nbytes;

    memset(dst->str, 0, dst->nbytes);        // NOLINT
    memcpy(dst->str, src->str, src->nbytes); // NOLINT
    // strings must be null terminated
    if (dst->nbytes > 0)
        dst->str[dst->nbytes - 1] = '\0';
    return 1;
Error:
    return 0;
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
unit_test__sample_type_as_string__is_defined_for_all()
{
    for (int i = 0; i < SampleTypeCount; ++i) {
        // Check this isn't returning "unknown" for known counts
        CHECK(sample_type_as_string(i)[0] != '(');
    }
    return 1;
Error:
    return 0;
}

int
unit_test__bytes_of_type__is_defined_for_all()
{
    for (int i = 0; i < SampleTypeCount; ++i) {
        // Check this isn't returning 0 for known counts
        CHECK(bytes_of_type(i) != 0);
    }
    return 1;
Error:
    return 0;
}

int
unit_test__components__copy_string()
{
    const char* abcde = "abcde";
    const char* vwxyz = "vwxyz";
    const char* fghi = "fghi";
    const char* jklmno = "jklmno";

    struct String src = { .str = (char*)malloc(strlen(abcde) + 1),
                          .nbytes = strlen(abcde) + 1,
                          .is_ref = 1 };
    struct String dst = { .str = (char*)malloc(strlen(vwxyz) + 1),
                          .nbytes = strlen(vwxyz) + 1,
                          .is_ref = 0 };
    CHECK(src.str);
    CHECK(dst.str);

    // dst is_ref = 1
    // lengths equal
    memcpy(src.str, abcde, strlen(abcde) + 1);
    memcpy(dst.str, vwxyz, strlen(vwxyz) + 1);
    CHECK(copy_string(&dst, &src));
    // src should be unchanged
    CHECK(0 == strcmp(src.str, abcde));
    CHECK(strlen(abcde) + 1 == src.nbytes);
    CHECK(1 == src.is_ref);

    // dst should be identical to src, except is_ref
    CHECK(0 == strcmp(dst.str, src.str));
    CHECK(dst.nbytes == src.nbytes);
    CHECK(0 == dst.is_ref); // no matter what happens, this String is owned

    // copy longer to shorter
    memcpy(dst.str, fghi, strlen(fghi) + 1);
    dst.nbytes = strlen(fghi) + 1;
    dst.is_ref = 1;

    CHECK(copy_string(&dst, &src));
    CHECK(0 == strcmp(dst.str, src.str));
    CHECK(dst.nbytes == src.nbytes);
    CHECK(0 == dst.is_ref); // no matter what happens, this String is owned

    // copy shorter to longer
    memcpy(dst.str, jklmno, strlen(jklmno) + 1);
    dst.nbytes = strlen(jklmno) + 1;
    dst.is_ref = 1;

    CHECK(copy_string(&dst, &src));
    CHECK(0 == strcmp(dst.str, src.str));
    CHECK(dst.nbytes == src.nbytes);
    CHECK(0 == dst.is_ref); // no matter what happens, this String is owned

    // dst is_ref = 0
    // lengths equal
    memcpy(dst.str, vwxyz, strlen(vwxyz) + 1);
    dst.nbytes = strlen(vwxyz) + 1;
    dst.is_ref = 0;

    CHECK(copy_string(&dst, &src));
    // src should be unchanged
    CHECK(0 == strcmp(src.str, abcde));
    CHECK(strlen(abcde) + 1 == src.nbytes);
    CHECK(1 == src.is_ref);

    // dst should be identical to src, except is_ref
    CHECK(0 == strcmp(dst.str, src.str));
    CHECK(dst.nbytes == src.nbytes);
    CHECK(0 == dst.is_ref); // no matter what happens, this String is owned

    // copy longer to shorter
    memcpy(dst.str, fghi, strlen(fghi) + 1);
    dst.nbytes = strlen(fghi) + 1;
    dst.is_ref = 0;

    CHECK(copy_string(&dst, &src));
    CHECK(0 == strcmp(dst.str, src.str));
    CHECK(dst.nbytes == src.nbytes);
    CHECK(0 == dst.is_ref); // no matter what happens, this String is owned

    // copy shorter to longer
    free(dst.str);
    CHECK(dst.str = malloc(strlen(jklmno) + 1));
    memcpy(dst.str, jklmno, strlen(jklmno) + 1);
    dst.nbytes = strlen(jklmno) + 1;
    dst.is_ref = 0;

    CHECK(copy_string(&dst, &src));
    CHECK(0 == strcmp(dst.str, src.str));
    CHECK(dst.nbytes == src.nbytes);
    CHECK(0 == dst.is_ref); // no matter what happens, this String is owned

    free(src.str);
    free(dst.str);

    return 1;
Error:
    return 0;
}

#endif // NO_UNIT_TESTS