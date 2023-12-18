#include "logger.h"
#include <stdarg.h>
#include <stdio.h>

static struct
{
    acquire_reporter_t reporter;
} globals = { 0 };

void
logger_set_reporter(acquire_reporter_t reporter)
{
    globals.reporter = reporter;
}

void
aq_logger(int is_error,
          const char* file,
          int line,
          const char* function,
          const char* fmt,
          ...)
{
    acquire_reporter_t reporter = globals.reporter;
    if (reporter) {
        char buf[1024] = { 0 };
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap); // NOLINT
        va_end(ap);
        reporter(is_error, file, line, function, buf);
    }
}
