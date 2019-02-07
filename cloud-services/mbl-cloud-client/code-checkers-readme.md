clang-tidy description
http://releases.llvm.org/7.0.0/tools/clang/tools/extra/docs/clang-tidy/index.html

clang-format description:
https://clang.llvm.org/docs/ClangFormat.html

- We use clang-format as formater, not as checker. the tool just changes files accoriding to the provided configuration file. The proposed change is always correct.  

clang-format style configuration comprehansive explanation:
https://www.clangformat.com/


Prerequistes:

Toolchains installations
sudo apt-get install clang
sudo apt-get install clang-tidy
sudo apt-get install clang-format

Libraries installations
sudo apt-get install libsystemd-dev

Build

Download mbl-core repository in the folder <mbl-core> on your PC. 

Download mbed-cloud-client repository in to the `mbl-core/cloud-services/mbl-cloud-client/`. 
mbed-cloud-client release version should be the same as written in `https://github.com/ARMmbed/mbl-core/blob/mbl-core-preq331/cloud-services/mbl-cloud-client/README.md`

Copy `mbed_cloud_dev_credentials.c` and `update_default_resources.c` into the `mbl-core/cloud-services/mbl-cloud-client/` folder.

Run the following commands: 
`python pal-platform/pal-platform.py deploy --target=x86_x64_NativeLinux_mbedtls generate`

`cd __x86_x64_NativeLinux_mbedtls`

In the folder `__x86_x64_NativeLinux_mbedtls` run: 
`cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=./../pal-platform/Toolchain/GCC/GCC.cmake -DEXTARNAL_DEFINE_FILE=./../define.txt -DCODE_CHECK_MODE=ON`

For clang-format run on all mbl-cloud-client sources and mbl-cloud-client gtest run:
make clang-format

For clang-tidy run on all mbl-cloud-client sources and mbl-cloud-client gtest run:
make clang-tidy


