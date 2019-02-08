# clang-tidy description
http://releases.llvm.org/7.0.0/tools/clang/tools/extra/docs/clang-tidy/index.html

# clang-format description
https://clang.llvm.org/docs/ClangFormat.html
- We use clang-format as a  formatter, not as a checker.  The tool just changes files according to the provided configuration file. The proposed change is always correct.  
- a comprehensive explanation about clang-format style configuration file  read here https://www.clangformat.com/

# Prerequisites
## Toolchains installations
`sudo apt-get install clang`

`sudo apt-get install clang-tidy`

`sudo apt-get install clang-format`

## Libraries installations
`sudo apt-get install libsystemd-dev`

# Build
## Get the tree on your PC
- Download mbl-core repository to the folder <mbl-core> on your PC. 
- Download mbed-cloud-client repository to the `mbl-core/cloud-services/mbl-cloud-client/`. Pay attention, `mbed-cloud-client` release version should be the same as specified in `https://github.com/ARMmbed/mbl-core/blob/mbl-core-preq331/cloud-services/mbl-cloud-client/README.md`
- Copy `mbed_cloud_dev_credentials.c` and `update_default_resources.c` into the `mbl-core/cloud-services/mbl-cloud-client/` folder.

## Run the following commands 
`python pal-platform/pal-platform.py deploy --target=x86_x64_NativeLinux_mbedtls generate`

`cd __x86_x64_NativeLinux_mbedtls`

In the folder `__x86_x64_NativeLinux_mbedtls` run:

`cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=./../pal-platform/Toolchain/GCC/GCC.cmake -DEXTARNAL_DEFINE_FILE=./../define.txt -DCODE_CHECK_MODE=ON`

The following command generates makefiles with the corresponding rules for clang-tidy and clang-format running.

# Run tools
## Running tools on mbl-cloud-client and gtest sources (from the `__x86_x64_NativeLinux_mbedtls` folder)
### run clang-format in a code formatter mode
make clang-format
- The tool will change the content of the files. Changes can be reviewed by running `git diff`.

### clang-tidy
make clang-tidy
- The tool will print on the stdout the warnings on the code quality. Proposed changes can be viewed in clang-tidy-suggested-fixes.txt in the  `__x86_x64_NativeLinux_mbedtls` folder.

## Running tools on the specific source file (from the same `__x86_x64_NativeLinux_mbedtls` folder)
### clang-format in a code checker mode
clang-format -style=file /absolute_or_relative/path/to/file.cpp
- The tool will print fixed version of the file to the stdout.

### run clang-format in a code formatter mode
clang-format -i -style=file /absolute_or_relative/path/to/file.cpp
- The tool will change the content of the file. Changes can be reviewed by running `git diff`.

### clang-tidy in a code static analysys mode
clang-tidy -p=. -export-fixes=./clang-tidy-suggested-fixes.txt -checks=-*,bugprone-*,cert-*,cppcoreguidelines-*,clang-analyzer-*,modernize-*,performance-*,readability-* /absolute_or_relative/path/to/file.cpp
