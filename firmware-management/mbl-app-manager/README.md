# mbl-app-manager - Mbed Linux OS user application manager

`mbl-app-manager` is an application that manages the installation and removal user applications from an ipk on Mbed Linux OS.

It expects open package management (OPKG) packages known as `.ipk`s.

The update package may be delivered via a trusted delivery method using [`mbl-app-update-manager`](../mbl-app-update-manager).


## Installation and usage

### Installation

`mbl-app-manager` can be installed by running the following command when the current working directory is set to `mbl-core/firmware-management/mbl-app-manager`:
```
pip install .
```

## Usage

```
usage: mbl-app-manager [-h] [-v] {install,force-install,remove,list} ...
```

## Return code

| Code | Meaning                                           |
|------|---------------------------------------------------|
| 0    | Success                                           |
| 1    | An error occurred                                 |
| 2    | Incorrect usage of the application                |

## Command line options

```
optional arguments:
  -h, --help            show this help message and exit
  -v, --verbose         increase verbosity of status information (default:
                        False)

subcommands:
  The commands to control the application statuses.

  {install,force-install,remove,list}
    install             install a user application.
    force-install       remove a previous installation of a user application
                        before installing.
    remove              remove a user application.
    list                list installed applications.
```

```
usage: mbl-app-manager install [-h] app_package app_path

positional arguments:
  app_package  application package to install a user application.
  app_path     path to install the application.

optional arguments:
  -h, --help   show this help message and exit
```

```
usage: mbl-app-manager force-install [-h] app_package app_path

positional arguments:
  app_package  application package to install a user application.
  app_path     path to install the app.

optional arguments:
  -h, --help   show this help message and exit
```

```
usage: mbl-app-manager remove [-h] app_name app_path

positional arguments:
  app_name    name of the user application to remove.
  app_path    path the app was installed.

optional arguments:
  -h, --help  show this help message and exit
```

```
usage: mbl-app-manager list [-h] apps_path

positional arguments:
  apps_path   path to look for installed applications.

optional arguments:
  -h, --help  show this help message and exit
```

## License

Please see the [License][mbl-license] document for more information.


## Contributing

Please see the [Contributing][mbl-contributing] document for more information.


[mbl-license]: ../LICENSE.md
[mbl-contributing]: ../CONTRIBUTING.md
