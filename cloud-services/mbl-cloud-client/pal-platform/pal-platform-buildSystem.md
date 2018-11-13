# pal-platform Build System

`NOTE!`
If you are not familiar with CMAKE please go to [CMAKE documantation](https://cmake.org/documentation/)

The Build system is designed to be flexible as possible and allows each component the flexibility to change it's sources without any changes in other components.

The pal-platform build system is constructed from a subset of cmake macros.
The sources for the build system macros is located in [common.cmake](./common.cmake).

## The Build system Macros.
1. ADDSUBDIRS() - This macro scans all the child directories for a CMakeLists.txt file. If exists call the cmake command 'add_subdirectory' on the child directory.
2. CREATE_LIBRARY(library_name, source_files, defines) - This macro creates library file with the name 'library_name' from the sources 'source_files' with the compile defines elborated in 'defined' compiled into an *.obj file. This file will be linked with all other libraries.
3. CREATE_TEST_LIBRARY(library_name, source_files, defines) - This macro creates library file with the name 'library_name' from the sources 'source_files' with the compile defines elborated in 'defined' compiled into an *.obj file. This library will linked to all libraries created by CREATE_LIBRARY macro and will create a binary file.
4. ADD_GLOBALDIR(dir_name) - This macro adds a directory "dir_name"  to the include path for all the components. The macro writes the directory inside a file with '-I', the file is added to the C and CXX flags in the toolchain-flags files. [See example GCC-flags file.](Toolchain/GCC/GCC-flags.cmake#L148-#L153)

## How to use the Build system.
After deploying the relevant target using the [pal-platform util](./pal-platform-util.md) you will have a new directory named "__targetName" inside this directory there will be a CMakeFile.txt which is actually [mbedCloudClientCmake.txt](./mbedCloudClientCmake.txt). This file gathers all the libraries created by CREATE_LIBRARY and links them with the each of the CREATE_TEST_LIBRARY to create a binary file. 
Each CMakeLists.txt file needs to call ADDSUBDIRS macro to include his child directories.

Please view  [mbed-client-pal](https://github.com/ARMmbed/mbed-client-pal) repository for example:
 1. main [CMakeLists.txt](https://github.com/ARMmbed/mbed-client-pal/blob/master/CMakeLists.txt) file
 2. source [CMakeLists.txt](https://github.com/ARMmbed/mbed-client-pal/blob/master/Source/CMakeLists.txt) file
 3. Tests [CMakeLists.txt](https://github.com/ARMmbed/mbed-client-pal/blob/master/Test/CMakeLists.txt) file

 