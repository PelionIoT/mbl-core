# mbl-firmware-update-manager - Mbed Linux OS firmware update manager

`mbl-firmware-update-manager` is an application that manages the installation of root filesystem as well as user application from an update package on Mbed Linux OS.

It generates the necessary header data from the update package then delegates the installation of the content of the update package to a shell script. 
Installation of base platform components such as boot loaders, the kernel or rootfs may be performed using suitable installer shell scripts.

The update package may be delivered via Pelion Device management or via some other trusted delivery method.

`mbl-firmware-update-manager` is responsible for:
* creating the header data from the update package
* add the header data to an header file
* delegate the installation of the content of the update package
* reboot the device after a successful update if requested

`mbl-firmware-update-manager` requires the following Python packages to be installed on Mbed Linux OS:
* [`mbl-app-update-manager`](../mbl-app-update-manager)
* [`mbl-firmware-update-header-util`](../mbl-firmware-update-header-util)

## Installation and usage

### Installation

`mbl-firmware-update-manager` can be installed by running:
```
pip install .
```

## Usage
```
usage: mbl-firmware-update-manager [-h] [-r] [-v] <update-package>

MBL firmware update manager

positional arguments:
  <update-package>  update package containing firmware to install
```

## Return code

| Code | Meaning                                           |
|------|---------------------------------------------------|
| 0    | Success - firmware correctly installed            |
| 1    | An error occurred                                 |
| 2    | Incorrect usage of the application                |

## Command line options

```
optional arguments:
  -h, --help        show this help message and exit
  -r, --reboot      reboot after firmware update (default: False)
  -v, --verbose     Increase output verbosity (default: False)
```

## License

Please see the [License][mbl-license] document for more information.


## Contributing

Please see the [Contributing][mbl-contributing] document for more information.


[mbl-license]: ../LICENSE.md
[mbl-contributing]: ../CONTRIBUTING.md