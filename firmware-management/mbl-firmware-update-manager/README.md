# Firmware Update Manager

Manages the firmware that runs on a device. Provides a public method to accept
an update file for installation.  The update file may have been delivered via
the Mbed Update service or via some other trusted delivery method.
The firmware management component is responsible for unpacking the update file
and delegating installation to separate handlers.  A handler for installing
applications should be included.  Installation of base platform components
such as boot loaders, the kernel or RootFS may be performed using suitable
installer shell scripts.

Different installers are needed for installing different components. Some
installers will require boot loader support in order to apply an update during
boot. The update process is required to be robust to power and communication
failures and should support rollback to the previous version if an update
fails. A record of installed firmware should be maintained to support device
management operations to query the state of device firmware.

