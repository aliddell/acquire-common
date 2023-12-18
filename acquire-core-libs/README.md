# Acquire Core Libraries

[![Tests](https://github.com/acquire-project/acquire-core-libs/actions/workflows/test_pr.yml/badge.svg)](https://github.com/acquire-project/acquire-core-libs/actions/workflows/test_pr.yml)

This builds the core static libraries that get used in other parts of Acquire.
For the most part, these are interfaces private to Acquire, although there are
some exceptions.

- **acquire-core-logger** Logging facility.
- **acquire-core-platform** Enables compatibility across operating systems.
- **acquire-device-properties** Defines the parameters used for configuring devices.
- **acquire-device-kit** Defines the API needed for implementing a new device driver.
- **acquire-device-hal** Defines the API the runtime uses for interacting with drivers and devices.

## Acquire core logger

Acquire reports debug and trace events by printing them through a "logger".
This library defines what that is and provides a function for setting a global
callback at runtime that handles incoming messages.

This involves very little code. It barely merits being its own library. It's
here to define what the expected callback interface is - something almost every
component needs to know about.

## Acquire core platform

The Acquire core platform library defines a platform-independent interface for
interacting with the linux, osx, or windows operating systems.

This is used internally with certain Acquire components.

## Acquire device properties

This library defines many of the types that the Acquire device interfaces rely on to configure and query devices.

## Acquire Device Kit

This is a header only library. Some types are defined outside this library -
it needs to be combined with `acquire-device-properties` to be fully defined.

Acquire relies on device adapters to define video sources, filters and sinks.
The Device Kit defines the interfaces that need to be implemented to create a
device adapter.

Each device adapter is compiled to a shared library that exposes a single
function, `acquire_driver_init_v0()`. That function returns a "Driver"
instance. Here, a Driver's responsibility is to list a number of devices that
can be opened and to manage those devices as resources. It also manages those
resources.

The "Devices" themselves may implement specific interfaces depending on their
Kind. See `storage.h` and `camera.h` for details.

## Acquire Hardware Abstraction Library (HAL)

The Acquire HAL defines internal (private) functionality that the runtime uses
to interface with different kinds of devices.
