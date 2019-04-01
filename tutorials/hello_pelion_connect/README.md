# Purpose

The purpose of the `hello-pelion-connect` sample application is to demonstrate Pelion Connect D-Bus service methods' invocation from a containerized application which runs on the device. Demonstrated methods:
1. RegisterResources - Pelion Connect D-Bus API method which registers LwM2M resources provided by the application in the Device Management Portal.

# Mbed Linux OS hello-pelion-connect sample application 

This tutorial creates a user application that runs on MBL devices. It is an application written in Python, which demonstrates how an application should call the Pelion Connect D-Bus API methods.

<span class="notes">**Note:** Your device must already be running an MBL image. Please [follow the tutorial](https://os.mbed.com/docs/linux-os/v0.5/getting-started/tutorial-building-an-image.html) if you don't have an MBL image yet.</span>

# Prerequisites

 * Host PC OS should be Ubuntu 16.04.
 * Docker-CE should be installed on the host PC. Please refer [installing Docker-CE on Ubuntu][install-docker].
 * In order to build Docker image, an Internet connection is required on Host PC.

# Structure

`<mbl-core>/tutorials/hello_pelion_connect` directory which contains the following software:
- hello-pelion-connect application sources. The source code of the application is located in the `hello-pelion-connect` folder. The hello-pelion-connect application description please see in this [readme file][hello-pelion-connect-readme].
- Docker environment (shell scripts, makefile and the Dockerfile) for the containerized hello-pelion-connect application IPK creation. Shell scripts are located in the `<mbl-core>/tutorials/hello_pelion_connect/scripts` folder. Dockerfile which builds containerized `hello_pelion_connect` application resides in the root folder. Dockerfile which builds docker that is responsible for the IPK generation is located in the `cc-env` folder. Makefile which creates rootfs for the OCI container and generates the final IPK is located in the root folder. 
- OPKG configuration file which is required for IPK generation. OPKG configuration file is located under `src_opkg` folder.
- OCI container configuration file is located under `src_bundle` folder.



# Dockerizing the application and packing int the IPK file

On the device, the application runs on a target inside an OCI container. To create the OCI container, we dockerize the [`hello-pelion-connect`](hello-pelion-connect/) python package on a host machine. After that, the image is packed into the IPK file.  

Run the following command to dockerize the application and generate IPK file:
```
sh scripts/dockerize-hello-pelion-connect-app.sh
```
The script builds a docker image which includes an installation of the hello-pelion-connect application. The filesystem contained in the image is then exported and extracted to `release/runtime-bundle-filesystem`. The script builds a Docker image with dockcross, adding the `opkg-utils` utility. 

The final artifact of the [`dockerize-hello-pelion-connect-app.sh`](scripts/dockerize-hello-pelion-connect-app.sh) script is the OCI bundle that is packed in the IPK file at `release/ipk/hello-pelion-connect_1.0_armv7vet2hf-neon.ipk`.

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

After installation, and after every reboot, the application performs the functionality described in this.

MBL redirects the logging information to the log file `/var/log/app/hello-pelion-connect.log`.

### Restart hello-pelion-connect
In order to restart the hello-pelion-connect application, kill the current hello-pelion-connect OCI container, and start the new one: 

```
# kill the current container run
mbl-app-lifecycle-manager -k hello-pelion-connect

# start new container run
mbl-app-lifecycle-manager --run-container hello-pelion-connect --application-id hello-pelion-connect --verbose
```

## Analyzing result
### Status success 
On the successful operation, expect log lines informing about the progress of the operation being printed to the STDOUT without any error messages. On the successful execution, RegisterResources method result is the registration of the application resources in the Pelion Device Management portal. [This section][hello-pelion-connect-readme-analyzing-result] explains how to verify resource registration in the Pelion portal. 

### Status failure 
If during the application execution any error has occurred, execution will be aborted and the error will be printed to the device's standard error output. 

[over-the-air-firmware-update]: https://os.mbed.com/docs/mbed-linux-os/v0.5/getting-started/tutorial-updating-mbl-devices-and-applications.html
[mbl-cli-flash]: https://os.mbed.com/docs/mbed-linux-os/v0.5/tools/device-update.html#update-an-application
[install-docker]: https://docs.docker.com/install/linux/docker-ce/ubuntu
[hello-pelion-connect-readme]: ./hello-pelion-connect/README.md

[hello-pelion-connect-readme-analyzing-result]: ./hello-pelion-connect/README.md#analyzing-result
