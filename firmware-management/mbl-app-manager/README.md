# mbl-app-manager - Mbed Linux OS user application manager

`mbl-app-manager` is an application that manages the installation and removal user applications from an ipk on Mbed Linux OS.

It expects an open package management (OPKG) packages known as `.ipk`s.

The update package may be delivered via a trusted delivery method using [`mbl-app-update-manager`](../mbl-app-update-manager).


## Installation and usage

### Installation

`mbl-app-manager` can be installed by running:
```
pip install .
```

## Usage
```
usage: mbl-app-manager [arguments] [<file>|<app>]
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
  -i <file>, --install-app <file>
                        package of the application to install. (default: None)
  -f <file>, --force-install-app <file>
                        remove an application if previously installed before
                        installing (default: None)
  -r <app>, --remove-app <app>
                        remove installed application where <app> is the name
                        of the application (default: None)
  -l, --list-installed-apps
                        list installed applications. (default: False)
  -v, --verbose         print application status information (default: False)
```
