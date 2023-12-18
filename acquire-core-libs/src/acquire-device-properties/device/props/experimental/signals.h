#ifndef H_ACQUIRE_PROPS_SIGNALS_V0
#define H_ACQUIRE_PROPS_SIGNALS_V0

#include "../components.h"
#include "../metadata.h"

struct Channel
{
    enum SampleType sample_type;
    enum SignalType signal_type;
    enum SignalIOKind signal_io_kind;
    struct VoltageRange voltage_range;
    uint8_t line; // logical line id
    char display_name[64];
};

struct SignalProperties
{
    struct signal_properties_channels_s
    {
        uint8_t line_count;
        struct Channel lines[32];
    } channels;
    struct signal_properties_timing_s
    {
        uint8_t terminal;
        enum TriggerEdge edge;
        struct SampleRateHz samples_per_second;
    } timing;
    struct signal_properties_triggers_s
    {
        uint8_t line_count;
        struct Trigger lines[32];
    } triggers;
};

struct SignalPropertyMetadata
{
    struct signal_properties_metadata_channels_s
    {
        uint8_t line_count;

        char display_names[32][32];
        size_t logical_ids[32];
        uint32_t input;
        uint32_t output;

        uint64_t supported_sample_types;
    } channels;
};

#endif // H_ACQUIRE_PROPS_SIGNALS_V0
