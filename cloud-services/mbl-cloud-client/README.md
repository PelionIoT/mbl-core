# Mbed Linux OS Cloud Client

## Purpose

`mbl-core/cloud-services/mbl-cloud-client/` folder contains the Cloud client for Mbed Linux OS.

In order to get an access to ARM's Cloud Services from Mbed Linux OS, Mbed Linux OS Cloud Client should be built and run as part of Mbed Linux OS.

For more information about Mbed Linux OS, please see [meta-mbl][meta-mbl].

## Mbed cloud client version

Mbed Cloud client current version is [2.1.1][cc-2-1-1].

## Issues

* The mbed-cloud-client library provides error codes asynchronously without any context to determine which request actually failed. This will make it hard to provide services to multiple processes, and may cause issues with tracking the registration state of the device.

* The configuration and CMake files were copied from <https://github.com/ARMmbed/mbed-cloud-client-example-internal> with minimal modification and should be checked to see whether everything in them is actually suitable for Mbed Linux OS.

## License

Please see the [License][mbl-license] document for more information.

## Contributing

Please see the [Contributing][mbl-contributing] document for more information.



[cc-2-1-1]: https://github.com/ARMmbed/mbed-cloud-client/releases/tag/2.1.1
[meta-mbl]: https://github.com/ARMmbed/meta-mbl/blob/master/README.md
[mbl-license]: LICENSE
[mbl-contributing]: CONTRIBUTING.md
