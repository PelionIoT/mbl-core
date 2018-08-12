# Add new port to pal-platform

Adding a new port to pal-platform includes three steps:
- Adding to pal-platform deploy and generate python script
- Adding Toolchain configurations
- Adding CMake files to the platform components

### Adding port to pal platfrom script
To Add a new port to the python script update the [pal-platfrom json file](./pal-platform.json), by filling in the target name and components.

For the full build option you will need to update the toolchain lookup table at https://github.com/ARMmbed/pal-platform/blob/master/pal-platform.py#L915-L921.



### Adding Toolchain configurations
Adding a new toolchain requires the creation of a new folder containing two files:
1.  Toolchain - a file with basic configurations for the toolcahin.
    For example please view [GCC file](./Toolchain/GCC/GCC.cmake)
2.  Flags file - a file that contains all the compilation flags for this specific Toolchain
    For example please view [GCC Flags file](./Toolchain/GCC/GCC-flags.cmake)

`Please Note`
1. The value of TOOLCHAIN_FLAGS_FILE inside the Toolchain file must be set to the toolchain flags file.
See example inside the GCC toolchain file: https://github.com/ARMmbed/pal-platform/blob/master/Toolchain/GCC/GCC.cmake#L82

2. The flags file must contain an include to include.txt file for the ADD_GLOBALDIR CMake macro to work.
See example inside the GCC-flags toolchain file:
https://github.com/ARMmbed/pal-platform/blob/master/Toolchain/GCC/GCC-flags.cmake#L150-L153

### Adding CMake files to platform

Every component added to the pal-platfrom should have its own cmake file. It does not matter if this is a SDK, Device or middleware.

 At the beginning of the main CMake file it collects all the components cmake files and run them one by one. The variables that were set in this stage will affect all the CMake chain.