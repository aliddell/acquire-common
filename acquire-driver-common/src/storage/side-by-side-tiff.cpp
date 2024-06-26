// A side by side tiff is a folder where metadata and video data are stored in
// files that sit "side-by-side".
//
// ## Example
// Layout of files for a two camera acquisition:
//
// ```
//<filename>/metadata0.json
//           stream0.tif
//           metadata1.json
//           stream1.tif
//```

#include "device/kit/storage.h"
#include "device/props/storage.h"
#include "platform.h"
#include "logger.h"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

#define L (aq_logger)
#define LOG(...) L(0, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOGE(...) L(1, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define EXPECT(e, ...)                                                         \
    do {                                                                       \
        if (!(e)) {                                                            \
            LOGE(__VA_ARGS__);                                                 \
            throw std::runtime_error("Expression was false: " #e);             \
        }                                                                      \
    } while (0)
#define CHECK(e) EXPECT(e, "Expression evaluated as false:\n\t%s", #e)

#define containerof(ptr, T, V) ((T*)(((char*)(ptr)) - offsetof(T, V)))

extern "C" struct Storage*
tiff_init();

namespace {

struct SideBySideTiff
{
    struct Storage storage;
    struct Storage* tiff;
    StorageProperties props;
};

fs::path
as_path(const StorageProperties& props)
{
    return { props.uri.str, props.uri.str + props.uri.nbytes - 1 };
}

void
validate_write_permissions(const fs::path& path)
{
    const auto perms = fs::status(path).permissions();
    EXPECT(((perms & fs::perms::group_write) != fs::perms::none) ||
             ((perms & fs::perms::owner_write) != fs::perms::none) ||
             ((perms & fs::perms::others_write) != fs::perms::none),
           "Expected \"%s\" to have write permissions.",
           path.c_str());
}

void
validate_json(const char* str, size_t nbytes)
{
    if (!str || !nbytes)
        return;
    // Don't do full json validation here, but make sure it at least
    // begins and ends with '{' and '}'
    EXPECT(nbytes >= 3,
           "nbytes (%d) is too small. Expected a null-terminated json string.",
           (int)nbytes);
    EXPECT(str[nbytes - 1] == '\0', "String must be null-terminated");
    EXPECT(str[0] == '{', "json string must start with \'{\'");
    EXPECT(str[nbytes - 2] == '}', "json string must end with \'}\'");
}

void
validate(const struct StorageProperties* props)
{
    validate_json(props->external_metadata_json.str,
                  props->external_metadata_json.nbytes);

    {
        CHECK(props->uri.str);
        CHECK(props->uri.nbytes);

        const size_t offset = strlen(props->uri.str) >= 7 &&
                                  strncmp(props->uri.str, "file://", 7) == 0
                                ? 7
                                : 0;

        fs::path path(props->uri.str + offset,
                      (props->uri.str + offset) + (props->uri.nbytes - offset));
        auto parent_path = path.parent_path();
        if (parent_path.empty()) {
            parent_path = fs::path(".");
        }
        EXPECT(fs::is_directory(parent_path),
               "Expected \"%s\" to be a directory.",
               parent_path.c_str());
        validate_write_permissions(parent_path);
    }
}

enum DeviceState
side_by_side_tiff_set(struct Storage* self_,
                      const struct StorageProperties* props) noexcept
{
    try {
        CHECK(self_);
        struct SideBySideTiff* self =
          containerof(self_, struct SideBySideTiff, storage);
        validate(props);
        CHECK(storage_properties_copy(&self->props, props));

        const size_t offset = strlen(props->uri.str) >= 7 &&
                                  strncmp(props->uri.str, "file://", 7) == 0
                                ? 7
                                : 0;

        // update the URI if it has a file:// prefix
        if (offset) {
            const char* filename = props->uri.str + offset;
            const size_t nbytes = props->uri.nbytes - offset;
            CHECK(storage_properties_set_uri(&self->props, filename, nbytes));
        }

    } catch (const std::exception& e) {
        LOGE("Exception: %s\n", e.what());
        return DeviceState_AwaitingConfiguration;
    } catch (...) {
        LOGE("Exception: (unknown)");
        return DeviceState_AwaitingConfiguration;
    }
    return DeviceState_Armed;
}

void
side_by_side_tiff_get(const struct Storage* self_,
                      struct StorageProperties* props) noexcept
{
    struct SideBySideTiff* self =
      containerof(self_, struct SideBySideTiff, storage);
    *props = self->props;
}

void
side_by_side_tiff_get_meta(const struct Storage* self_,
                           struct StoragePropertyMetadata* meta) noexcept
{
    struct SideBySideTiff* self =
      containerof(self_, struct SideBySideTiff, storage);
    try {
        CHECK(self->tiff);
        self->tiff->get_meta(self->tiff, meta);
    } catch (const std::exception& e) {
        LOGE("Exception: %s\n", e.what());
    } catch (...) {
        LOGE("Exception: (unknown)");
    }
}

enum DeviceState
side_by_side_tiff_start(struct Storage* self_) noexcept
{
    DeviceState state = DeviceState_AwaitingConfiguration;
    try {
        CHECK(self_);
        struct SideBySideTiff* self =
          containerof(self_, struct SideBySideTiff, storage);

        // 1. create folder
        const auto path = as_path(self->props);
        if (fs::exists(path)) {
            EXPECT(
              fs::is_directory(path), "%s must be a directory.", path.c_str());
        } else {
            EXPECT(fs::create_directory(path),
                   "Failed to create folder for \"%s\"",
                   path.c_str());
        }

        // 2. write metadata.json file
        if (self->props.external_metadata_json.nbytes) {
            const auto metadata_path =
              (path / "metadata.json").generic_string();
            struct file file
            {};
            CHECK(file_create(
              &file, metadata_path.c_str(), metadata_path.length()));
            const auto is_ok =
              file_write(&file,
                         0,
                         (uint8_t*)self->props.external_metadata_json.str,
                         (uint8_t*)self->props.external_metadata_json.str +
                           self->props.external_metadata_json.nbytes - 1);
            file_close(&file);
            EXPECT(is_ok, "Write to %s failed.", metadata_path.c_str());
        }

        // 3. set/start tiff writer
        {
            const auto video_path = (path / "data.tif").generic_string();
            StorageProperties props{};
            storage_properties_copy(&props, &self->props);
            props.uri = {
                .str = (char*)video_path.c_str(),
                .nbytes = video_path.length(),
                .is_ref = 1,
            };
            CHECK(self->tiff);
            state = self->tiff->set(self->tiff, &props);
            CHECK(state == DeviceState_Armed);
            state = self->tiff->start(self->tiff);
            CHECK(state == DeviceState_Running);
        }

    } catch (const std::exception& e) {
        LOGE("Exception: %s\n", e.what());
        state = DeviceState_AwaitingConfiguration;
    } catch (...) {
        LOGE("Exception: (unknown)");
        state = DeviceState_AwaitingConfiguration;
    }
    return state;
}

enum DeviceState
side_by_side_tiff_stop(struct Storage* self_) noexcept
{
    try {
        CHECK(self_);
        struct SideBySideTiff* self =
          containerof(self_, struct SideBySideTiff, storage);
        CHECK(self->tiff);
        CHECK(self->tiff->stop(self->tiff) == DeviceState_Armed);
    } catch (const std::exception& e) {
        LOGE("Exception: %s\n", e.what());
        return DeviceState_AwaitingConfiguration;
    } catch (...) {
        LOGE("Exception: (unknown)");
        return DeviceState_AwaitingConfiguration;
    }
    return DeviceState_Armed;
}

void
side_by_side_tiff_destroy(struct Storage* self_) noexcept
{
    try {
        CHECK(self_);
        struct SideBySideTiff* self =
          containerof(self_, struct SideBySideTiff, storage);
        CHECK(self->tiff);
        if (self_->stop)
            self_->stop(self_);
        self->tiff->destroy(self->tiff);
    } catch (const std::exception& e) {
        LOGE("Exception: %s\n", e.what());
    } catch (...) {
        LOGE("Exception: (unknown)");
    }
}

void
side_by_side_tiff_reserve_image_shape(struct Storage* self_,
                                      const struct ImageShape* shape) noexcept
{ // no-op
}

enum DeviceState
side_by_side_tiff_append(struct Storage* self_,
                         const struct VideoFrame* frame,
                         size_t* nbytes) noexcept
{
    try {
        CHECK(self_);
        struct SideBySideTiff* self =
          containerof(self_, struct SideBySideTiff, storage);
        CHECK(self->tiff);
        CHECK(self->tiff->append(self->tiff, frame, nbytes) ==
              DeviceState_Running);
    } catch (const std::exception& e) {
        LOGE("Exception: %s\n", e.what());
        return side_by_side_tiff_stop(self_);
    } catch (...) {
        LOGE("Exception: (unknown)");
        return side_by_side_tiff_stop(self_);
    }
    return DeviceState_Running;
}

} // end ::{anonymous} namespace

extern "C" struct Storage*
side_by_side_tiff_init()
{
    struct SideBySideTiff* self = (struct SideBySideTiff*)malloc(sizeof(*self));
    *self = { .storage = {
        .set = side_by_side_tiff_set,
        .get = side_by_side_tiff_get,
        .get_meta = side_by_side_tiff_get_meta,
        .start = side_by_side_tiff_start,
        .append = side_by_side_tiff_append,
        .stop = side_by_side_tiff_stop,
        .destroy = side_by_side_tiff_destroy,
        .reserve_image_shape = side_by_side_tiff_reserve_image_shape,
        },
        .tiff=tiff_init()
    };
    return &self->storage;
}
