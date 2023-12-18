#define _GNU_SOURCE

#include "platform.h"
#include "logger.h"

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/file.h>

#define L (aq_logger)
#define LOG(...) L(0, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOGE(...) L(1, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define TRACE(...)
#define EXPECT(e, ...)                                                         \
    do {                                                                       \
        if (!(e)) {                                                            \
            LOGE(__VA_ARGS__);                                                 \
            goto Error;                                                        \
        }                                                                      \
    } while (0)
#define CHECK(e) EXPECT(e, "Expression evaluated as false:\n\t%s", #e)
#define CHECK_POSIX(ecode)                                                     \
    do {                                                                       \
        int ecode_ = 0;                                                        \
        if ((ecode_ = (ecode)) != 0) {                                         \
            const char* emsg = strerror(ecode_);                               \
            LOGE("Expression returned error code %d: %s",                      \
                 ecode_,                                                       \
                 (emsg ? emsg : "(bad error code)"));                          \
            goto Error;                                                        \
        }                                                                      \
    } while (0)

int
file_create(struct file* file, const char* filename, size_t bytesof_filename)
{
    file->fid = open(filename, O_RDWR | O_CREAT | O_NONBLOCK, 0666);
    if (file->fid < 0) {
        CHECK_POSIX(errno);
    } else {
        int ret = flock(file->fid, LOCK_EX | LOCK_NB);
        if (ret < 0) {
            LOGE("Failed to create existing file \"%s\"", filename);
            int tmp = errno;
            close(file->fid);
            CHECK_POSIX(tmp);
        }
    }
    return 1;
Error:
    LOGE("Failed to create \"%s\"", filename);
    return 0;
}

void
file_close(struct file* file)
{
    if (close(file->fid) < 0)
        CHECK_POSIX(errno);
Error:;
}

int
file_write(const struct file* file,
           uint64_t offset,
           const uint8_t* cur,
           const uint8_t* end)
{
    int retries = 0;
    while (cur < end && retries < 3) {
        size_t remaining = end - cur;
        ssize_t written = pwrite(file->fid, cur, remaining, offset);
        if (written < 0) {
            CHECK_POSIX(errno);
        }
        retries += (written == 0);
        offset += written;
        cur += written;
    }
    return (retries < 3);
Error:
    return 0;
}

int
file_exists(const char* filename, size_t nbytes)
{
    int ret = access(filename, F_OK);
    if (ret < 0) {
        if (errno == ENOENT)
            return 0;
        CHECK_POSIX(errno);
    }
    return ret == 0;
Error:
    return 0;
}

int
file_is_writable(const char* filename, size_t nbytes)
{
    if (file_exists(filename, nbytes)) {
        int ret = access(filename, W_OK);
        if (ret < 0)
            CHECK_POSIX(errno);
    } else {
        // file doesn't exist, try to create
        int fid = open(filename, O_RDWR | O_CREAT | O_NONBLOCK, 0666);
        if (fid < 0)
            CHECK_POSIX(errno);
        close(fid);
        unlink(filename);
    }
    return 1;
Error:
    LOGE("path \"%s\" not writable", filename);
    return 0;
}

void*
memory_alloc(size_t capacity_bytes, enum AllocatorHint hint)
{
    return malloc(capacity_bytes);
}

void
memory_free(void* address)
{
    free(address);
}

void
clock_init(struct clock* clock)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    clock->origin = (uint64_t)(t.tv_sec * 1e9) + (uint64_t)t.tv_nsec;
}

#ifndef NO_UNIT_TESTS
int
unit_test__monotonic_clock_increases_monotonically()
{
    struct clock t, s;
    clock_init(&t);
    clock_init(&s);

    EXPECT(t.origin <= s.origin,
           "Expected clock t <= s. Got %llu > %llu",
           (unsigned long long)t.origin,
           (unsigned long long)s.origin);
    return 1;
Error:
    return 0;
}
#endif

void
clock_shift_ms(struct clock* clock, double ms)
{
    int64_t dt = (int64_t)(ms * 1e6); // clock tics are in ns
    if (ms < 0 && clock->origin < -dt) {
        clock->origin = 0;
    } else {
        clock->origin += dt;
    }
}

uint64_t
clock_tic(struct clock* clock)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    if (clock)
        clock->origin = (uint64_t)(1e9 * t.tv_sec) + (uint64_t)t.tv_nsec;
    return (uint64_t)(1e9 * t.tv_sec) + (uint64_t)t.tv_nsec;
}

int64_t
clock_toc(struct clock* clock)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    const int64_t dt =
      (uint64_t)(t.tv_sec * 1e9) + (uint64_t)t.tv_nsec - clock->origin;
    return dt;
}

double
clock_toc_ms(struct clock* clock)
{
    // clock tics are in ns
    return (double)(clock_toc(clock) * 1e-6);
}

int8_t
clock_cmp(struct clock* clock, uint64_t timestamp)
{
    const uint64_t o = clock->origin;
    return (timestamp < o) ? -1 : ((timestamp > o) ? 1 : 0);
}

int8_t
clock_cmp_now(struct clock* clock)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    const uint64_t now = (uint64_t)(t.tv_sec * 1e9) + (uint64_t)t.tv_nsec;
    return clock_cmp(clock, now);
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
        const int seconds = (int)(1e-3 * remaining_ms);
        const int nsec = (int)(1e6f * (remaining_ms - 1e3f * (float)seconds));
        const struct timespec t = { .tv_sec = seconds, .tv_nsec = nsec };
        TRACE("\nsleep delay: %g ms - remaining: %g ms - %d %d",
              (double)delay_ms,
              (double)remaining_ms,
              seconds,
              nsec);

        nanosleep(&t, 0);
        clock_tic(clock);
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
    self->inner_ = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
}

void
lock_acquire(struct lock* self)
{
    CHECK_POSIX(pthread_mutex_lock(&self->inner_));
Error:;
}

int
try_lock_acquire(struct lock* self)
{
    return 0 == pthread_mutex_trylock(&self->inner_);
}

void
lock_release(struct lock* self)
{
    CHECK_POSIX(pthread_mutex_unlock(&self->inner_));
Error:;
}

void
condition_variable_init(struct condition_variable* self)
{
    self->inner_ = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
}

void
condition_variable_wait(struct condition_variable* restrict self,
                        struct lock* restrict lock)
{
    CHECK_POSIX(pthread_cond_wait(&self->inner_, &lock->inner_));
Error:;
}

void
condition_variable_notify_all(struct condition_variable* self)
{
    CHECK_POSIX(pthread_cond_broadcast(&self->inner_));
Error:;
}

void
event_init(struct event* self)
{
    *self = (struct event){
        .lock_ = PTHREAD_MUTEX_INITIALIZER,
        .cond_ = PTHREAD_COND_INITIALIZER,
        .state_ = 0,
    };
}

void
event_destroy(struct event* self)
{
    // no op
}

void
event_notify_all(struct event* self)
{
    CHECK_POSIX(pthread_mutex_lock(&self->lock_));
    self->state_ = 1;
    CHECK_POSIX(pthread_cond_broadcast(&self->cond_));
    CHECK_POSIX(pthread_mutex_unlock(&self->lock_));
Error:;
}

void
event_wait(struct event* self)
{
    CHECK_POSIX(pthread_mutex_lock(&self->lock_));
    while (!self->state_) {
        CHECK_POSIX(pthread_cond_wait(&self->cond_, &self->lock_));
    }
    self->state_ = 0; // reset
    CHECK_POSIX(pthread_mutex_unlock(&self->lock_));
Error:;
}

void
thread_init(struct thread* self)
{
    self->inner_ = 0;
    self->is_live_ = 0;
    self->lock_ = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
}

uint8_t
thread_create(struct thread* self, void (*proc)(void*), void* args)
{
    uint8_t is_ok = 1;
    pthread_mutex_lock(&self->lock_);
    self->is_live_ = 1;
    CHECK_POSIX(pthread_create(&self->inner_, 0, (void* (*)(void*))proc, args));
Finalize:
    pthread_mutex_unlock(&self->lock_);
    return is_ok;
Error:
    is_ok = 0;
    self->is_live_ = 0;
    goto Finalize;
}

void
thread_join(struct thread* self)
{
    // pthread_join() will indefinitely block on threads handles that have
    // already been joined. The `is_live_` flag is used to track whether
    // the thread handle is joinable.
    pthread_mutex_lock(&self->lock_);
    if (self->is_live_) {
        void* v;
        CHECK_POSIX(pthread_join(self->inner_, &v));
        self->is_live_ = 0;
    }
Error:
    pthread_mutex_unlock(&self->lock_);
    return;
}

int
lib_open(struct lib* self, const char* absolute_path)
{
    CHECK(self);
    EXPECT(self->inner = dlopen(absolute_path, RTLD_NOW | RTLD_LOCAL),
           "Failed to load %s. Error: %s",
           absolute_path,
           dlerror());
    return 1;
Error:
    return 0;
}

void
lib_close(struct lib* self)
{
    if (self && self->inner) {
        EXPECT(dlclose(self->inner) == 0,
               "Failed to close library (%p). Error: %s",
               self->inner,
               dlerror());
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
    EXPECT(out = dlsym(self->inner, name),
           "Failed to load symbol \"%s\" from library %p. Error: %s",
           name,
           self->inner,
           dlerror());
    return out;
Error:
    return out;
}

// Returns the absolute path to the module containing this function.
// Return value must be freed by caller
static char*
path_to_current_module()
{
    Dl_info info = { 0 };
    dladdr(__FUNCTION__, &info);
    if (!info.dli_fname)
        return 0;
    char* out = realpath(info.dli_fname, 0);
    char* slash = 0;
    if (out && (slash = strrchr(out, '/'))) {
        *slash = '\0'; // truncate path at file name
        return out;
    } else {
        LOGE("Could not truncate filename in path \"%s\"", out);
        free(out);
        return 0;
    }
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
    EXPECT(out = malloc(nbytes + 1), "Failed to allocate %llu bytes", nbytes);
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
    char *root = 0, *absolute_path = 0;
    CHECK(root = path_to_current_module());
    const char* parts[] = { root, "/lib", name, ".so", NULL };
    CHECK(absolute_path = join(parts));
    const int out = lib_open(self, absolute_path);
    free(root);
    free(absolute_path);
    return out;
Error:
    self->inner = 0;
    if (root)
        free(root);
    if (absolute_path)
        free(absolute_path);
    return 0;
}
