#ifndef H_ACQUIRE_METADATA_V0
#define H_ACQUIRE_METADATA_V0

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    enum PropertyType
    {
        PropertyType_FixedPrecision,
        PropertyType_FloatingPrecision,
        PropertyType_Enum,
        PropertyType_String,
    };

    struct Property
    {
        uint8_t writable;
        float low, high;
        enum PropertyType type;
    };

#ifdef __cplusplus
}
#endif

#endif // H_ACQUIRE_METADATA_V0

/* NOTES:
    - genicam properties have attributes like "access mode" (RW or RO),
      "streamable", "visibility", "is locked", "min", "max", and "inc".
      Several of these are relational: for example, "min" refers to another
      property name.
 */
