#ifndef H_ACQUIRE_LOGGER_V0
#define H_ACQUIRE_LOGGER_V0

#ifdef __cplusplus
extern "C"
{
#endif

    typedef void (*acquire_reporter_t)(int is_error,
                                       const char* file,
                                       int line,
                                       const char* function,
                                       const char* msg);

    /// @brief Set the global reporter callback.
    /// @param[in] reporter Callback used to log messages. May be NULL.
    /// @see acquire_reporter_ts
    void logger_set_reporter(acquire_reporter_t reporter);

    void aq_logger(int is_error,
                   const char* file,
                   int line,
                   const char* function,
                   const char* fmt,
                   ...);

#ifdef __cplusplus
}
#endif

#endif // H_ACQUIRE_LOGGER_V0
