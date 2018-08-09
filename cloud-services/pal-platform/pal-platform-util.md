# pal-platform utility

This utility script deploys and generates platform-dependent files needed to build and run non-mbed OS
applications.

The script can run on Linux or Windows.

Currently, the only supported target is a native Linux over an x86 machine using mbed TLS.

Deployment information for supported targets is described in [`pal-platform.json`](../pal-platform.json).

## Table of contents

- [Requirements](#requirements)
- [Usage](#usage)
  - [Deploy](#deploy)
  - [Generate](#generate)
  - [Clean](#clean)
- [Examples](#examples)
  - [x86_x64_NativeLinux_mbedtls](#x86_x64_nativelinux_mbedtls)

## Requirements

* [Python 2.7 or higher](https://www.python.org/downloads) or [Python 3.4 or higher](https://www.python.org/downloads).
* [Git 2](https://git-scm.com/).
* Windows only: the `patch.exe` utility (part of [Git for Windows](https://git-scm.com/download/win)).

  **Note:** This must be in the PATH.

* Python modules:
  * **`click`**
    * run `pip install click` (Add `sudo` when running on Linux).
  * **`requests`**
    * run `pip install requests` (Add `sudo` when running on Linux).

## Usage

The script gets its default deployment information from [`pal-platform.json`](../pal-platform.json), but you can
give your own custom deployment information *.json* file using the `--from-file` flag.

```bash
~/dev/mbed-testapp/pal-platform$ python pal-platform.py -h
Usage: pal-platform.py [OPTIONS] COMMAND1 [ARGS]... [COMMAND2 [ARGS]...]...

Options:
  -v, --verbose     Turn ON verbose mode
  --from-file PATH  Path to a .json file containing the supported targets configuration.
                    Default is ~/dev/mbed-testapp/pal-platform/pal-platform.json
  --version         Show the version and exit.
  -h, --help        Show this message and exit.

Commands:
  clean     Clean target-dependent files (run "pal-platform.py clean -h" for help)
  deploy    Deploy mbed-cloud-client files (run "pal-platform.py deploy -h" for help)
  generate  Generate target-dependent files (run "pal-platform.py generate -h" for help)
```

### Deploy

This sub-command deploys platform-dependent files for a given target. It fetches the files from their source location
to their required destination in the `pal-platform` folder and applies patches as needed.

```bash
~/dev/mbed-testapp/pal-platform$ python pal-platform.py deploy -h
Usage: pal-platform.py deploy [OPTIONS]

  Deploy target-dependent files

Options:
  --target [x86_x64_NativeLinux_mbedtls]
                                  The target to deploy platform-dependent files for  [required]
  --skip-update                   Skip Git Repositories update
  -i, --instructions              Show deployment instructions for a given target and exit.
  -h, --help                      Show this message and exit.
```

#### Deployment instructions

If you want to see a target's deployment instructions, without deploying it, use the `-i/--instructions` flag and the instructions will be printed to *stdout*.

For example, on the `x86_x64_NativeLinux_mbedtls` target:

```bash
~/dev/mbed-testapp/pal-platform$ python pal-platform.py deploy --target=x86_x64_NativeLinux_mbedtls -i
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ x86_x64_NativeLinux_mbedtls ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#################
# mbedtls  2.4.0
#################
- Clone git@github.com:ARMmbed/mbedtls.git at master to ~/dev/mbed-testapp/pal-platform/Middleware/mbedtls/mbedtls
```

### Generate

This sub-command generates the target's build directory in the form of `__<TARGET_NAME>` on the same level as the
`pal_platform` directory. The build directory contains two files that CMake will use later: `CMakeLists.txt` and `autogen.cmake`.

```bash
~/dev/mbed-testapp/pal-platform$ python pal-platform.py generate -h
Usage: pal-platform.py generate [OPTIONS]

  Generate files to be used by build-system

Options:
  --target [x86_x64_NativeLinux_mbedtls]
                                  The target to generate platform-dependent files for
  -h, --help                      Show this message and exit.
```

You can generate and deploy with a single command by using both `generate` and `deploy`:

```
python pal-platform.py deploy --target=x86_x64_NativeLinux_mbedtls generate
```

When `generate` finishes running, you can run *cmake* and *make* from the generated build directory.

### Clean

This sub-command deletes the target's build directory and all the target's deployed files and directories.

**Tip:** You can use the `--keep-sources` flag to keep the target's deployed files and directories.

```bash
~/dev/mbed-testapp/pal-platform$ python pal-platform.py clean -h
Usage: pal-platform.py clean [OPTIONS]

  Clean target-dependent files

Options:
  --target [x86_x64_NativeLinux_mbedtls]
                                  The target to clean  [required]
  -k, --keep-sources              Keep the deployed platform-dependent files
                                  (clean only generated files)
  -h, --help                      Show this message and exit.
```

## Examples

### x86_x64_NativeLinux_mbedtls

**x86_x64_NativeLinux_mbedtls** content in `pal-platform.json`:
```json
  "x86_x64_NativeLinux_mbedtls":
  {
    "device": {
      "name": "x86_x64"
    },
    "os": {
      "name": "Linux_Native"
    },
    "middleware": {
      "mbedtls": {
        "version": "2.4.2",
        "from": {
          "protocol": "git",
          "location": "git@github.com:ARMmbed/mbedtls.git",
          "tag": "mbedtls-2.4.2"
        },
        "to": "Middleware/mbedtls/mbedtls"
      }
    }
  },
```

**deploy**:

```bash
~/dev/mbed-testapp/pal-platform$ python pal-platform.py deploy --target=x86_x64_NativeLinux_mbedtls
2017-04-23 09:40:59,038 - pal-platform - INFO - Getting mbedtls from git
2017-04-23 09:40:59,038 - pal-platform - INFO - Cloning from git@github.com:ARMmbed/mbedtls.git at master to ~/dev/mbed-testapp/pal-platform/Middleware/mbedtls/mbedtls
Deployment for x86_x64_NativeLinux_mbedtls is successful.
Deployment instructions are in ~/dev/mbed-testapp/pal-platform/x86_x64_NativeLinux_mbedtls.txt.
```

**generate**:

```bash
~/dev/mbed-testapp/pal-platform$ python pal-platform.py generate --target=x86_x64_NativeLinux_mbedtls
2017-04-23 14:00:42,196 - pal-platform - INFO - Generated ~/dev/mbed-testapp/__x86_x64_NativeLinux_mbedtls/autogen.cmake
Generation for x86_x64_NativeLinux_mbedtls is successful, please run cmake & make from ~/dev/mbed-testapp/__x86_x64_NativeLinux_mbedtls
```

**clean**:

```bash
~/dev/mbed-testapp/pal-platform$ python pal-platform.py clean --target=x86_x64_NativeLinux_mbedtls
2017-04-23 14:02:47,325 - pal-platform - INFO - Deleting ~/dev/mbed-testapp/pal-platform/__x86_x64_NativeLinux_mbedtls
2017-04-23 14:02:47,331 - pal-platform - INFO - Deleting ~/dev/mbed-testapp/pal-platform/Middleware/mbedtls/mbedtls
```
