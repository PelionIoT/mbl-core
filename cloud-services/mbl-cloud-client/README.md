# Mbed Linux OS Cloud Client

## Purpose

`mbl-core/cloud-services/mbl-cloud-client/` folder contains the Cloud client for Mbed Linux OS.

In order to get an access to ARM's Cloud Services from Mbed Linux OS, Mbed Linux OS Cloud Client should be built and run as part of Mbed Linux OS.

For more information about Mbed Linux OS, please see [meta-mbl][meta-mbl].

## Mbed cloud client version

Mbed Cloud client current version is [3.0.0][cc-3-0-0].

## Log trace level

Log trace level can be set in the function log_init() in log.cpp file.

Valid options are:

TRACE_ACTIVE_LEVEL_ALL    - used to activate all trace levels
TRACE_ACTIVE_LEVEL_DEBUG  - print all traces same as above
TRACE_ACTIVE_LEVEL_INFO   - print info,warn and error traces
TRACE_ACTIVE_LEVEL_WARN   - print warn and error traces
TRACE_ACTIVE_LEVEL_ERROR  - print only error trace
TRACE_ACTIVE_LEVEL_CMD    - print only cmd line data
TRACE_ACTIVE_LEVEL_NONE   - trace nothing

## Issues

* The mbed-cloud-client library provides error codes asynchronously without any context to determine which request actually failed. This will make it hard to provide services to multiple processes, and may cause issues with tracking the registration state of the device.

* The configuration and CMake files were copied from <https://github.com/ARMmbed/mbed-cloud-client-example-internal> with minimal modification and should be checked to see whether everything in them is actually suitable for Mbed Linux OS.

## License

Please see the [License][mbl-license] document for more information.

## Contributing

Please see the [Contributing][mbl-contributing] document for more information.



[cc-3-0-0]: https://github.com/ARMmbed/mbed-cloud-client/releases/tag/3.0.0
[meta-mbl]: https://github.com/ARMmbed/meta-mbl/blob/master/README.md
[mbl-license]: LICENSE
[mbl-contributing]: CONTRIBUTING.md
