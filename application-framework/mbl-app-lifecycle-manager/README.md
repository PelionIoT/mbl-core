# mbl-app-lifecycle-manager - Mbed Linux OS user application lifecycle manager

`mbl-app-lifecycle-manager` is concerned with starting, stopping user applications runtime bundles. The OCI runtime used is `runC` which is included in Mbed Linux OS.

## Installation and usage

### Installation

`mbl-app-lifecycle-manager` can be installed by running:

```
pip install .
```

## Usage

```
usage: mbl-app-lifecycle-manager [-h] [-v] {run,run-all,terminate,kill} ...
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

  {run,run-all,terminate,kill}
    run                 run a user application found at /home/app.
    run-all             run all user applications found at /home/app.
    terminate           kill a user application process with SIGTERM then
                        delete its associated resources. Note: A SIGKILL is
                        sent if the application does not stop after a SIGTERM.
    kill                kill a user application process with SIGKILL then
                        delete its associated resources.
```

```
usage: mbl-app-lifecycle-manager run [-h] app_name app_path

positional arguments:
  app_name    name the application will be referred as once it has been
              started.

optional arguments:
  -h, --help  show this help message and exit
```

```
usage: mbl-app-lifecycle-manager run-all [-h]

optional arguments:
  -h, --help  show this help message and exit
```

```
usage: mbl-app-lifecycle-manager terminate [-h] [-t SIGTERM_TIMEOUT]
                                           [-k SIGKILL_TIMEOUT]
                                           app_name

positional arguments:
  app_name              name of the application to terminate.

optional arguments:
  -h, --help            show this help message and exit
  -t SIGTERM_TIMEOUT, --sigterm-timeout SIGTERM_TIMEOUT
                        timeout to wait for an application to stop after
                        SIGTERM.
  -k SIGKILL_TIMEOUT, --sigkill-timeout SIGKILL_TIMEOUT
                        timeout to wait for an application to stop after
                        SIGKILL.
```

```
usage: mbl-app-lifecycle-manager kill [-h] [-t SIGKILL_TIMEOUT] app_name

positional arguments:
  app_name              name of the application to kill.

optional arguments:
  -h, --help            show this help message and exit
  -t SIGKILL_TIMEOUT, --sigkill-timeout SIGKILL_TIMEOUT
                        timeout to wait for an application to stop after
                        SIGKILL.
```

## License

Please see the [License][mbl-license] document for more information.


## Contributing

Please see the [Contributing][mbl-contributing] document for more information.


[mbl-license]: ../LICENSE.md
[mbl-contributing]: ../CONTRIBUTING.md
