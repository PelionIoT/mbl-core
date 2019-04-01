# Mbed Linux OS hello-pelion-connect sample application 

The purpose of the `hello-pelion-connect` sample application is to demonstrate Pelion Connect D-Bus service methods' invocation from a containerized application which runs on the device. Demonstrated methods:
1. RegisterResources - Pelion Connect D-Bus API method which registers LwM2M resources provided by the application in the Device Management Portal.

This tutorial creates a user application that runs on MBL devices. It is an application written in Python, which demonstrates how an application should call the Pelion Connect D-Bus API methods.

<span class="notes">**Note:** Your device must already be running an MBL image. Please [follow the tutorial](https://os.mbed.com/docs/linux-os/v0.5/getting-started/tutorial-building-an-image.html) if you don't have an MBL image yet.</span>

# Prerequisites

 * Host PC running Linux (it was only tested on Ubuntu 16.04)
 * Docker-CE should be installed on the host PC. Please refer [installing Docker-CE on Ubuntu][install-docker].
 * In order to build Docker image, an Internet connection is required on host PC.

# Structure

The application is contained as a sub-project within `mbl-core` at [`mbl-core/tutorials/hello_pelion_connect`](https://github.com/ARMmbed/mbl-core/tree/master/tutorials/hello_pelion_connect).
The sub-project has the following:
- The source code of the application
- [Dockerfile](https://github.com/ARMmbed/mbl-core/tree/master/tutorials/hello_pelion_connect/Dockerfile) to containerize the application.
- Helper scripts that to facilitate the creation of the containerized application.
- [Dockerfile](https://github.com/ARMmbed/mbl-core/tree/master/tutorials/hello_pelion_connect/cc-env/Dockerfile) to create a container where the tools to create an IPK are installed.
- Makefile which uses the container mentioned above to create an IPK containing the containerized application.
- OPKG configuration file which is required for IPK generation. OPKG configuration file is located under `src_opkg` folder.
- OCI container configuration file is located under `src_bundle` folder.



# Dockerizing the application and packing int the IPK file

On the device, the application runs on a target inside an OCI container. To create the OCI container, we dockerize the [`hello-pelion-connect`](hello-pelion-connect/) python package on a host machine. After that, the image is packed into the IPK file.  

Run the following command to dockerize the application and generate IPK file:
```
sh scripts/dockerize-hello-pelion-connect-app.sh
```

The script builds a docker image which includes an installation of the `hello-pelion-connect` application. The filesystem contained in the image is then exported and extracted to `release/runtime-bundle-filesystem`. The script also builds a Docker image with `opkg-utils` utility used later on by the Makefile to create an IPK. 

The final artifact of the [`dockerize-hello-pelion-connect-app.sh`](scripts/dockerize-hello-pelion-connect-app.sh) script is the OCI bundle that is packed in the IPK file at `release/ipk/hello-pelion-connect_1.0_any.ipk`.

## Cleaning the build artifacts

Run the following command to remove the `release` directory:
```
sh scripts/dockerize-hello-pelion-connect-app.sh clean
``` 

## Notes

* The `release` directory is created by the [`dockerize-hello-pelion-connect-app.sh`](scripts/dockerize-hello-pelion-connect-app.sh) script.

## Installing and running the application on the device

To install the application on the device:

* [Send the application as an over-the-air firmware update with Device Management](https://os.mbed.com/docs/linux-os/v0.5/getting-started/tutorial-updating-mbl-devices-and-applications.html).
* [Flash the application over USB with MBL CLI](https://os.mbed.com/docs/linux-os/v0.5/tools/device-update.html#update-an-application).

After installation, and after every reboot, the application is started.

MBL redirects the logging information to the log file `/var/log/app/hello-pelion-connect.log`.

### Restart hello-pelion-connect
In order to restart the hello-pelion-connect application, kill the current hello-pelion-connect OCI container, and start the new one: 

```
# terminate the running application
mbl-app-lifecycle-manager -v terminate hello-pelion-connect

# start the application
mbl-app-lifecycle-manager -v run hello-pelion-connect
```

## Analyzing result
### Status success 
On the successful operation, expect log lines informing about the progress of the operation being printed to the STDOUT without any error messages. On the successful execution, RegisterResources method result is the registration of the application resources in the Pelion Device Management portal. [This section][hello-pelion-connect-readme-analyzing-result] explains how to verify resource registration in the Pelion portal. 

### Status failure 
If during the application execution any error has occurred, execution will be aborted and the error will be printed to the device's standard error output. 

[over-the-air-firmware-update]: https://os.mbed.com/docs/mbed-linux-os/master/update/updating-an-application.html
[mbl-cli-flash]: https://os.mbed.com/docs/mbed-linux-os/master/update/updating-an-application.html#using-mbl-cli
[install-docker]: https://docs.docker.com/install/linux/docker-ce/ubuntu
[hello-pelion-connect-readme-analyzing-result]: ./hello-pelion-connect/README.md#analyzing-result
