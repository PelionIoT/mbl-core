# Application Update Manager

Manages the applications that run on a Mbed Linux OS during an update. It
provides methods to stop, run, install, remove and identify applications
deployed in OCI containers. The update file may have been delivered via the
Mbed Update service or via some other trusted delivery method. The application
update manager component is responsible for unpacking the update file and
delegating installation to separate handlers. A handler for installing
applications should be included.


