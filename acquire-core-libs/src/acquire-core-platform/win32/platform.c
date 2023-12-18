// https://github.com/google/benchmark/blob/v1.1.0/src/cycleclock.h#L116

#include "platform.h"
#include "logger.h"

#include <stdint.h>
#include <math.h>

#define L aq_logger
#define LOG(...) L(0, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOGE(...) L(1, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

// #define TRACE(...) L(0, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define TRACE(...)

#define EXPECT_INNER(LOGGER, e, ...)                                           \
    do {                                                                       \
        if (!(e)) {                                                            \
            LOGGER(__VA_ARGS__);                                               \
            goto Error;                                                        \
        }                                                                      \
    } while (0)
#define EXPECT(e, ...) EXPECT_INNER(LOGE, e, __VA_ARGS__)
#define CHECK(e) EXPECT(e, "Expression evaluated as false:\n\t%s", #e)
#define EXPECT_SILENT(e, ...) EXPECT_INNER(TRACE, e, __VA_ARGS__)
#define CHECK_SILENT(e)                                                        \
    EXPECT_SILENT(e, "Expression evaluated as false:\n\t%s", #e)

#define CHECK_WARN(e)                                                          \
    do {                                                                       \
        if (!(e)) {                                                            \
            LOGE("Expression evaluated as false:\n\t%s\n\t%s", #e, errstr());  \
        }                                                                      \
    } while (0)

#define CHECK_HANDLE(e)                                                        \
    do {                                                                       \
        if ((e) == INVALID_HANDLE_VALUE) {                                     \
            LOGE(                                                              \
              "Expression evaluated to an invalid handle value:\n\t%s\n\t%s",  \
              #e,                                                              \
              errstr());                                                       \
            goto Error;                                                        \
        }                                                                      \
    } while (0)

static struct
{
    uint8_t is_large_page_support_enabled_;
} globals = { 0 };

static const char*
errstr()
{
    static char buf[1024] = { 0 };
    ZeroMemory(buf, sizeof(buf));
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
                   0,
                   GetLastError(),
                   0,
                   buf,
                   sizeof(buf) - 1,
                   0);
    return buf;
}

int
file_create(struct file* file, const char* filename, size_t bytes_of_filename)
{
    memset(file, 0, sizeof(*file));

    file->overlapped.hEvent = CreateEvent(0, TRUE, FALSE, 0);
    CHECK(file->overlapped.hEvent != INVALID_HANDLE_VALUE);

    CHECK_HANDLE(file->hfile = CreateFileA(filename,
                                           GENERIC_WRITE,
                                           FILE_SHARE_READ,
                                           0,
                                           CREATE_ALWAYS,
                                           FILE_FLAG_OVERLAPPED,
                                           0));
    return 1;
Error:
    LOGE("Could not create \"%s\"", filename);
    return 0;
}

void
file_close(struct file* file)
{
    CHECK_WARN(CloseHandle(file->hfile));
    CHECK_WARN(CloseHandle(file->overlapped.hEvent));
    file->hfile = INVALID_HANDLE_VALUE;
    file->overlapped.hEvent = INVALID_HANDLE_VALUE;
}

int
file_write(const struct file* file,
           uint64_t offset,
           const uint8_t* cur,
           const uint8_t* end)
{
    int retries = 0;
    HANDLE hfile = file->hfile;
    OVERLAPPED ovl = file->overlapped;
    while (cur < end && retries < 3) {
        DWORD written = 0;
        DWORD remaining = (DWORD)(end - cur); // may truncate
        ovl.Pointer = (void*)offset;
        WriteFile(hfile, cur, (DWORD)remaining, 0, &ovl);
        CHECK(GetOverlappedResult(hfile, &ovl, &written, TRUE));
        retries += (written == 0);
        offset += written;
        cur += written;
    }
    return (retries < 3);
Error:
    return 0;
}

int
file_exists(const char* filename, size_t _nbytes)
{
    int out = 1;
    WIN32_FIND_DATAA query;
    HANDLE h = FindFirstFileA(filename, &query);
    if (h == INVALID_HANDLE_VALUE) {
        DWORD ecode = GetLastError();
        if (ecode == ERROR_FILE_NOT_FOUND) {
            out = 0;
        }
    }
    FindClose(h);
    return out;
}

int
file_is_writable(const char* filename, size_t nbytes)
{
    // Check if the file exists and is writable
    DWORD fileAttributes = GetFileAttributesA(filename);
    if (fileAttributes != INVALID_FILE_ATTRIBUTES &&
        !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        (fileAttributes & FILE_ATTRIBUTE_ARCHIVE ||
         fileAttributes & FILE_ATTRIBUTE_NORMAL)) {
        return 1;
    } else if (fileAttributes != INVALID_FILE_ATTRIBUTES &&
               fileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        return !(fileAttributes & FILE_ATTRIBUTE_READONLY);
    } else {
        // Check if the file can be created
        HANDLE h = CreateFileA(filename,
                               GENERIC_READ | GENERIC_WRITE,
                               0,
                               NULL,
                               CREATE_NEW,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
        if (h != INVALID_HANDLE_VALUE) {
            CloseHandle(h);
            return 1;
        }
    }
    LOGE("Could not open file for writing at \"%s\"", filename);
    return 0;
}

void*
mem_alloc_default(size_t capacity);

void*
mem_alloc_largepage(size_t capacity);

void*
memory_alloc(size_t capacity, enum AllocatorHint hint)
{
    switch (hint) {
        case AllocatorHint_Default:
            return mem_alloc_default(capacity);
        case AllocatorHint_LargePage:
            return mem_alloc_largepage(capacity);
        default:
            return 0;
    }
}

void*
mem_alloc_largepage(size_t capacity_)
{

    if (!globals.is_large_page_support_enabled_) {
        // Access control: Enable Large Page (2MB) Support
        LUID luid = { 0 };
        HANDLE token = { 0 };
        OpenProcessToken(
          GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &token);
        LookupPrivilegeValueA(NULL, "SeLockMemoryPrivilege", &luid);
        TOKEN_PRIVILEGES tp = {
            .PrivilegeCount = 1,
            .Privileges = { [0] = { .Luid = luid,
                                    .Attributes = SE_PRIVILEGE_ENABLED } }
        };
        globals.is_large_page_support_enabled_ |=
          (ERROR_SUCCESS ==
           AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), NULL, NULL));
    }

    void* buf = 0;
    if (globals.is_large_page_support_enabled_) {
        const size_t capacity = (capacity_ < GetLargePageMinimum())
                                  ? GetLargePageMinimum()
                                  : capacity_;

        buf = VirtualAlloc(NULL,
                           capacity,
                           MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES,
                           PAGE_READWRITE);
    }
    if (!buf) {
        buf = mem_alloc_default(capacity_);
    }
    return buf;
}

void*
mem_alloc_default(size_t capacity)
{
    return VirtualAlloc(
      NULL, capacity, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void
memory_free(void* address)
{
    VirtualFree(address, 0, MEM_RELEASE);
}

void
clock_init(struct clock* clock)
{
    QueryPerformanceFrequency(&clock->ticks_per_second);
    QueryPerformanceCounter(&clock->origin);
}

#ifndef NO_UNIT_TESTS
int
unit_test__monotonic_clock_increases_monotonically()
{
    struct clock t, s;
    clock_init(&t);
    clock_init(&s);

    // win32 only guarantees this is non-decreasing
    // See:
    // https://learn.microsoft.com/en-us/windows/win32/sysinfo/acquiring-high-resolution-time-stamps
    CHECK(t.origin.QuadPart <= s.origin.QuadPart);
    return 1;
Error:
    LOGE("Failed on %lld >= %lld", t.origin.QuadPart, s.origin.QuadPart);
    return 0;
}
#endif

void
clock_shift_ms(struct clock* clock, double ms)
{

    LARGE_INTEGER offset = {
        .QuadPart =
          (uint64_t)(clock->ticks_per_second.QuadPart * 1e-3 * fabs(ms)),
    };
    if (ms < 0) {
        const LARGE_INTEGER o = clock->origin;
        clock->origin.QuadPart =
          (o.QuadPart > offset.QuadPart) ? (o.QuadPart - offset.QuadPart) : 0;
    } else {
        clock->origin.QuadPart += offset.QuadPart;
    }
}

int8_t
clock_cmp_now(struct clock* clock)
{
    LARGE_INTEGER t = { 0 };
    QueryPerformanceCounter(&t);
    return clock_cmp(clock, t.QuadPart);
}

int8_t
clock_cmp(struct clock* clock, uint64_t timestamp)
{
    LARGE_INTEGER t = { .QuadPart = timestamp };
    return (t.QuadPart < clock->origin.QuadPart)
             ? -1
             : ((t.QuadPart > clock->origin.QuadPart) ? 1 : 0);
}

uint64_t
clock_tic(struct clock* clock)
{
    LARGE_INTEGER t, *pt = &t;
    if (clock)
        pt = &clock->origin;
    QueryPerformanceCounter(pt);
    return pt->QuadPart;
}

int64_t
clock_toc(struct clock* clock)
{
    LARGE_INTEGER t = { 0 };
    QueryPerformanceCounter(&t);
    return (uint64_t)(t.QuadPart - clock->origin.QuadPart);
}

double
clock_toc_ms(struct clock* clock)
{
    int64_t ms = clock_toc(clock) * 1000 / clock->ticks_per_second.QuadPart;
    return (double)ms;
}

void
clock_sleep_ms(struct clock* clock, float delay_ms)
{
    struct clock dummy;
    if (!clock) {
        clock_init(&dummy);
        clock = &dummy;
    }
    const float remaining_ms = delay_ms - (float)clock_toc_ms(clock);
    if (remaining_ms > 1.0f) {
        Sleep((DWORD)remaining_ms);
        clock_tic(clock);
    } else {
        Sleep(0);
    }
}

#ifndef NO_UNIT_TESTS
int
unit_test__clock_sleep_ms_accepts_null()
{
    // seg faults on fail
    clock_sleep_ms(0, 1);
    return 1;
}
#endif

void
lock_init(struct lock* self)
{
    InitializeSRWLock(&self->inner_);
}

void
lock_acquire(struct lock* self)
{
    AcquireSRWLockExclusive(&self->inner_);
}

int
try_lock_acquire(struct lock* self)
{
    return TryAcquireSRWLockExclusive(&self->inner_);
}

void
lock_release(struct lock* self)
{
    ReleaseSRWLockExclusive(&self->inner_);
}

void
condition_variable_init(struct condition_variable* self)
{
    InitializeConditionVariable(&self->inner_);
}

void
condition_variable_notify_all(struct condition_variable* self)
{
    WakeAllConditionVariable(&self->inner_);
}

void
condition_variable_wait(struct condition_variable* restrict self,
                        struct lock* restrict lock)
{
    SleepConditionVariableSRW(&self->inner_, &lock->inner_, INFINITE, 0);
}

void
event_init(struct event* self)
{
    self->inner_ = CreateEventA(0, 0, 0, 0);
}

void
event_destroy(struct event* self)
{
    CloseHandle(self->inner_);
}

void
event_notify_all(struct event* self)
{
    SetEvent(self->inner_);
}

void
event_wait(struct event* self)
{
    WaitForSingleObject(self->inner_, INFINITE);
}

void
thread_init(struct thread* self)
{
    self->inner_ = INVALID_HANDLE_VALUE;
}

uint8_t
thread_create(struct thread* self, void (*proc)(void*), void* args)
{
    CHECK(self->inner_ == INVALID_HANDLE_VALUE);
    self->inner_ = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)proc, args, 0, 0);
    CHECK(self->inner_ != INVALID_HANDLE_VALUE);
    return 1;
Error:
    return 0;
}

void
thread_join(struct thread* self)
{
    HANDLE thread = self->inner_; // FIXME: (nclack) ideally, this would be an
                                  // atomic compare exchange
    if (thread != INVALID_HANDLE_VALUE) {
        self->inner_ = INVALID_HANDLE_VALUE;
        TRACE("WFSO %p", thread);
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }
}

int
lib_open(struct lib* self, const char* absolute_path)
{
    CHECK(self);
    EXPECT_SILENT(self->inner = LoadLibraryA(absolute_path),
                  "Failed to load %s. Error: %s",
                  absolute_path,
                  errstr());
    TRACE("LOADED %s", absolute_path);
    return 1;
Error:
    return 0;
}

void
lib_close(struct lib* self)
{
    if (self && self->inner) {
        EXPECT(FreeLibrary(self->inner),
               "Failed to close library (%p). Error: %s",
               self->inner,
               errstr());
        self->inner = 0;
    }
Error:;
}

void*
lib_load(struct lib* self, const char* name)
{
    void* out = 0;
    CHECK(self && self->inner);
    CHECK(name);
    EXPECT(out = GetProcAddress(self->inner, name),
           "Failed to load symbol \"%s\" from library %p. Error: %s",
           name,
           self->inner,
           errstr());
    return out;
Error:
    return out;
}

// `strings` must be NULL-terminated.
// Caller must free the returned string.
static char*
join(const char** strings)
{
    char* out = 0;
    size_t nbytes = 0, nstrings = 0;
    for (const char** s = strings; *s; ++s) {
        nbytes += strlen(*s);
        nstrings += 1;
    }
    EXPECT(out = mem_alloc_default(nbytes + 1),
           "Failed to allocate %llu bytes",
           nbytes);
    char* cur = out;
    for (const char** s = strings; *s; ++s) {
        size_t n = strlen(*s);
        memcpy(cur, *s, n);
        cur += n;
    }
    *cur = '\0';

    return out;
Error:
    return 0;
}

int
lib_open_by_name(struct lib* self, const char* name)
{
    int is_ok = 1;
    char path[MAX_PATH] = { 0 };
    char* fullpath = 0;
    HMODULE hm = NULL;

    EXPECT(GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                              (LPCSTR) & __FUNCTION__,
                              &hm),
           "GetModuleHandle failed. Error: %s",
           errstr());

    EXPECT(GetModuleFileNameA(hm, path, sizeof(path)),
           "GetModuleFileName failed. Error: %s",
           errstr());

    {
        char* c = strrchr(path, '\\');
        if (c != NULL)
            c[1] = '\0';
    }

    const char* parts[] = { path, name, ".dll", NULL };
    CHECK(fullpath = join(parts));
    CHECK_SILENT(lib_open(self, fullpath));

Finalize:
    if (fullpath)
        VirtualFree(fullpath, 0, MEM_RELEASE);
    return is_ok;
Error:
    is_ok = 0;
    goto Finalize;
}
