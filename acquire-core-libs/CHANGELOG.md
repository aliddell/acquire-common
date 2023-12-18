# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed

- Users can specify the full chunk size in width, height, and planes.
- `acquire-device-hal`: `storage_open` no longer takes a `StorageProperties*` parameter.

### Removed

- `max_bytes_per_chunk` from `StorageProperties` chunking properties.

### Fixed

- Removes 30-second timeout from `thread_join` on Windows.
- Memory leak in `copy_string`.
- Avoid an unnecessary call to `realloc`.

### Added

- `acquire-device-properties`: A `uint8_t` member `enable_multiscale` of `StorageProperties`.
- `acquire-device-properties`: A corresponding entry in `StoragePropertyMetadata`.
- `acquire-device-properties`: A convenience function for enabling multiscale, similar to the function for
  setting chunking properties.
- `acquire-device-properties`: A `struct storage_properties_sharding_s` member `shard_dims_chunks` of
  `StorageProperties`. Users can now configure sharding properties where supported.
- `acquire-device-properties`: A convenience function for setting sharding parameters.
- `acquire-device-hal`: `storage_start`, `storage_stop`, and `storage_set` functions.

## [0.1.3](https://github.com/acquire-project/acquire-core-libs/compare/v0.1.2...v0.1.3) - 2023-06-27

### Changed

- `StorageProperties::chunking::max_bytes_per_chunk` is now a `uint64_t` (was a `uint32_t`).

## [0.1.2](https://github.com/acquire-project/acquire-core-libs/compare/v0.1.1...v0.1.2) - 2023-05-24

### Added

- Nightly releases.
- Device Properties: Storage property metadata.
- Device Properties: A function to set chunking parameters.
- Device Kit: A function to get property metadata for a storage device.
- Device Kit: A function to reserve image shape for a storage device.
- Device HAL: A function to get storage property metadata.
- Device HAL: A function to reserve image shape for a storage device.

### Changed

- Device Properties: The `storage_properties_init` function no longer accepts a `bytes_per_chunk` parameter.

### Fixed

- Compiles with position-independent code.

## 0.1.1 - 2023-05-11
