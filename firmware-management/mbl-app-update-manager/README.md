# mbl-app-update-manager - Mbed Linux OS user application update manager

`mbl-app-update-manager` is an application that manages the installation of user applications from an update package on Mbed Linux OS.

It expects a tar archive, referred here as the update package, which must contain one or multiple open package management (OPKG) packages with the `.ipk` extension.

The update package may be delivered via Pelion Device management or via some other trusted delivery method.

`mbl-app-update-manager` is responsible for:
* unpacking the update package
* verifiying the content of the update package
* installing new applications
* replacing currently installed version of applications

`mbl-app-update-manager` requires the following Python packages to be installed on Mbed Linux OS:
* [`mbl-app-manager`](../mbl-app-manager)
* [`mbl-app-lifecycle-manager`](../../application-framework/mbl-app-lifecycle-manager)

## Installation and usage

### Installation

`mbl-app-update-manager` can be installed by running:
```
pip install .
```

## Usage
```
usage: mbl-app-update-manager [-h] [-v] <update-package>

MBL application update manager

positional arguments:
  <update-package>  update package containing app(s) to install
```

## Return code

| Code | Meaning                                           |
|------|---------------------------------------------------|
| 0    | Success - application(s) correctly installed      |
| 1    | An error occurred                                 |
| 2    | Incorrect usage of the application                |

## Command line options

```
optional arguments:
  -h, --help        show this help message and exit
  -v, --verbose     increase verbosity of status information (default: False)
```

## License

Please see the [License][mbl-license] document for more information.


## Contributing

Please see the [Contributing][mbl-contributing] document for more information.


[mbl-license]: ../LICENSE.md
[mbl-contributing]: ../CONTRIBUTING.md
