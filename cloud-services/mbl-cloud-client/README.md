
# Mbed Linux OS Cloud Client

## Purpose

`mbl-core/cloud-services/mbl-cloud-client/` folder contains the Cloud client for Mbed Linux OS.

In order to get an access to ARM's Cloud Services from Mbed Linux OS, Mbed Linux OS Cloud Client should be built and run as part of Mbed Linux OS.

For more information about Mbed Linux OS, please see [meta-mbl][meta-mbl].

## Mbed cloud client version

Mbed Cloud client current version is [2.1.1][cc-2-1-1].

## Log trace level

Log trace level can be set in the function log_init() in log.cpp file.

Valid options are:

- TRACE_ACTIVE_LEVEL_ALL    - used to activate all trace levels
- TRACE_ACTIVE_LEVEL_DEBUG  - print all traces same as above
- TRACE_ACTIVE_LEVEL_INFO   - print info,warn and error traces
- TRACE_ACTIVE_LEVEL_WARN   - print warn and error traces
- TRACE_ACTIVE_LEVEL_ERROR  - print only error trace
- TRACE_ACTIVE_LEVEL_CMD    - print only cmd line data
- TRACE_ACTIVE_LEVEL_NONE   - trace nothing

## Development on an Ubuntu Linux on PC

### Prerequisites

- `mbed_cloud_dev_credentials.c`
  - A file containing credentials for your Mbed cloud account. (See
  [generate-a-developer-device-certificate][generate-a-developer-device-certificate] for instructions for how to obtain this file).
  - This file should be copied to the ```<mbl-core>/cloud-services/mbl-cloud-client``` direcotory.

- `update_default_resources.c`
  - A file containing a certificate for use by Update and some device information. For development purposes, this file can be generated using "manifest-tool init" (see [manifest-dev-tutorial][manifest-dev-tutorial]).
  - This file should be copied to the ```<mbl-core>/cloud-services/mbl-cloud-client``` direcotory.
- `mbed-cli`: Follow the instructions at [mbed-cli][mbed-cli] to install `mbed-cli`.
- The flllowing packages should be installed:

  ```bash
  sudo apt-get update
  sudo apt-get install python3-pip cmake libsystemd-dev libjsoncpp-dev build-essential
  sudo pip3 install requests click gitpython
  ```

### Building

Use ```client-builder.py``` script to build the MBL Cloud Client:

1. Download all dependancies and apply patches: ```client-builder.py -a prepare -m <META_MBL_REVISION>```
   ```<META_MBL_REVISION>``` is a git revision or branch of meta-mbl repository from where the script will download patches to apply.
1. Configure project (using cmake): ```client-builder.py -a configure```
1. Build/rebuild the project: ```client-builder.py -a build``` or ```client-builder.py -a rebuild```
   The difference between build and rebuild option is:
   - The ```build``` build only changed files.
   - The ```rebuild``` unconditionally build the whole project.

Note:
The execution order of those commands is important. The order should be:

1. ```prepare```
1. ```configure```
1. ```build``` or ```rebuild```

If a new files added to the project, need to run ```configure``` command before ```build``` or ```rebuild```.

### Running

TBD

### Clang (*Mandatory step before raising a PR)
clang-tidy is a clang-based C/C++ static analysis tool. Its purpose is to provide an extensible framework for diagnosing and fixing typical programming errors, like style violations, interface misuse, or bugs that can be deduced via static analysis. This tool does not enforce C/C++ style. Read more about the clang-tidy tool.

#### Prerequisites
##### Toolchains installations
```sudo apt-get install -y clang clang-tidy clang-format```

##### Libraries for mbl-cloud-client on PC compilation installations
Install all required libraries in order to compile mbl-cloud-client code (among which systemd development library).
```sudo apt-get install -y libsystemd-dev libjsoncpp-dev```
libjsoncpp-dev installs the header to /usr/include/jsoncpp/json/ while bitbake expects it to be under /usr/include/json directory.
In order to enable both - make a soft link: ```sudo ln -s /usr/include/jsoncpp/json/ /usr/include/json```

#### Usage
Use ```client-builder.py``` script to run clang code checkers on the MBL Cloud Client:
1. Prepare clang (using cmake): ```client-builder.py -a clang_configure```
2. Run clang tidy: ```client-builder.py -a clang_tidy```. In case of errors / warnings you will set a list of the issues needs to be handled. 
In order to run clang tidy need to run prepare step first.
3. Run clang format: ```client-builder.py -a clang_format```. Note: This will modify the files in case bad formating. Make sure you review the modified changes.
In order to run clang format need to run prepare step first.


### Optional installation of Visual Studio Code IDE

1. Install [Visual Studio Code IDE][vs-code-installaiton]
1. Install the following Visual Studio Code plugins:
   - C/C++
   - Doxygen Documentation Generator
   - GitHub
   - markdownlint
   - Toggle Header/Source
   - Visual Studio Keymap
   - Python
1. In order to build the project:
   - Launch Visual Studio Code IDE
   - Press <kbd>control</kbd>+<kbd>shift</kbd>+<kbd>B</kbd> and chose command to run

## Issues

- The mbed-cloud-client library provides error codes asynchronously without any context to determine which request actually failed. This will make it hard to provide services to multiple processes, and may cause issues with tracking the registration state of the device.

- The configuration and CMake files were copied from <https://github.com/ARMmbed/mbed-cloud-client-example-internal> with minimal modification and should be checked to see whether everything in them is actually suitable for Mbed Linux OS.

## License

Please see the [License][mbl-license] document for more information.

## Contributing

Please see the [Contributing][mbl-contributing] document for more information.

[generate-a-developer-device-certificate]: [https://cloud.mbed.com/docs/v1.2/quick-start/connecting-your-device-to-mbed-cloud.html#generate-a-developer-device-certificate]
[manifest-dev-tutorial]: [https://cloud.mbed.com/docs/v1.2/updating-firmware/manifest-dev-tutorial.html]
[mbed-cli]: [https://github.com/ARMmbed/mbed-cli]
[vs-code-installaiton]: https://code.visualstudio.com/docs/setup/linux
[cc-2-1-1]: https://github.com/ARMmbed/mbed-cloud-client/releases/tag/2.1.1
[meta-mbl]: https://github.com/ARMmbed/meta-mbl/blob/master/README.md
[mbl-license]: LICENSE
[mbl-contributing]: CONTRIBUTING.md 
