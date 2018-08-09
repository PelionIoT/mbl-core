# Mbed Linux Cloud Client Repository

## Purpose

`mbl-core/cloud-services` folder contains the Cloud client for Mbed Linux OS.

In order to get an access to ARM's Cloud Services from Mbed Linux, Mbed Linux Cloud Client should be built and run as part of Mbed Linux. To do that, follow the instructions at <https://github.com/ARMmbed/meta-mbl/blob/master/docs/walkthrough.md>

For more information about Mbed Linux OS, please see [meta-mbl][meta-mbl].

## Cloud Services

The Cloud Services component is concerned with providing device applications with access to Mbed Cloud services and for providing the process environment for running the Mbed Cloud Client.  To achieve this, upper-edge C/C++ APIs provided by the Mbed Cloud Client are mapped to a set of D-Bus interfaces to form a D-Bus service. 

## Mbed cloud client version

Mbed Cloud client current version is [1.4.0][cc-1-4-0].

## Prerequisites

* A GCC toolchain.

* `mbed_cloud_dev_credentials.c`: A file containing credentials for your Mbed cloud account. (See <https://cloud.mbed.com/docs/v1.2/quick-start/connecting-your-device-to-mbed-cloud.html#generate-a-developer-device-certificate> for instructions for how to obtain this file).

* `update_default_resources.c`: A file containing a certificate for use by Update and some device information. For development purposes, this file can be generated using "manifest-tool init" (see <https://cloud.mbed.com/docs/v1.2/updating-firmware/tutorial-creating-a-firmware-manifest-for-development.html#automated-workflows>).

* `mbed-cli`: Follow the instructions at <https://github.com/ARMmbed/mbed-cli> to install `mbed-cli`.

* Docker: see <https://www.docker.com>.

* Python

* CMake

## Building

1. Clone <https://github.com/ARMmbed/mbl-core> repository and navigate to the `cloud-services` folder that is located in the root folder of the mbl-core repository.

2. Copy `mbed_cloud_dev_credentials.c` and `update_default_resources.c` into the `cloud-services` folder.

3. Obtain dependencies using `mbed-cli` - from the repo's top level directory:
```shell
mbed deploy --protocol ssh
```

4. Configure:
```shell
python pal-platform/pal-platform.py deploy --target=x86_x64_NativeLinux_mbedtls generate
cd __x86_x64_NativeLinux_mbedtls
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=./../pal-platform/Toolchain/GCC/GCC.cmake -DEXTARNAL_DEFINE_FILE=./../define.txt
```

5. Build (still in the `__x86_x64_NativeLinux_mbedtls` directory):
```shell
make mbl-cloud-client
```

## Running

The executable uses various directories that you probably don't want to exist on your development machine so it is best to run mbed Linux Cloud Client in a docker container. While still in the `__x86_x64_NativeLinux_mbedtls` directory:
```shell
cp ../Dockerfile Debug
docker build -t mbl-cloud-client Debug
docker run -ti --rm mbl-cloud-client
```
That will start a bash session inside a container. To run the cloud client inside the container and follow the log file, just run
```shell
/opt/arm/mbl-cloud-client
tail -f /var/log/mbl-cloud-client.log
```

## Maintainers

* Jonathan Haigh <jonathan.haigh@arm.com>

## Issues

* The mbed-cloud-client library provides error codes asynchronously without any context to determine which request actually failed. This will make it hard to provide services to multiple processes, and may cause issues with tracking the registration state of the device.

* The configuration and CMake files were copied from <https://github.com/ARMmbed/mbed-cloud-client-example-internal> with minimal modification and should be checked to see whether everything in them is actually suitable for Mbed Linux OS.

## License

Please see the [License][mbl-license] document for more information.

## Contributing

Please see the [Contributing][mbl-contributing] document for more information.



[cc-1-4-0]: https://github.com/ARMmbed/mbed-cloud-client/releases/tag/1.4.0
[meta-mbl]: https://github.com/ARMmbed/meta-mbl/blob/master/README.md
[mbl-license]: LICENSE
[mbl-contributing]: CONTRIBUTING.md
