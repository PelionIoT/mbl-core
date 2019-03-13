# Purpose

To demonstrate Pelion Connect D-Bus service methods' invocation from the containerized application which runs on the device. Demonstrated methods:
1. RegisterResources - Pelion Connect D-Bus API method which registers LwM2M resources provided by the application in the Device Management Portal.

# Prerequisites

 * Host PC OS should be Ubuntu 16.04.
 * Docker-CE should be installed on the host PC. Please refer [installing Docker-CE on Ubuntu][install-docker].
 * In order to build Docker image, an Internet connection is required on Host PC.

# Structure

`<mbl-core>/tutorials/hello_pelion_connect` directory which contains the following software:
- hello-pelion-connect application sources. The source code of the application is located in the folder `source`. hello-pelion-connect application description please see in this [readme file][hello-pelion-connect-readme].
- Docker stuff (shell scripts and the Docker file) for the containerized hello-pelion-connect application IPK creation. Shell scripts are located in the root folder (`<mbl-core>/tutorials/hello_pelion_connect`). `Dockerfile` is located in the folder `cc-env`. 
- OPKG configuration which is required for IPK generation. OPKG configuration is located in `src_opkg` folder.  
- OCI container configuration which defines properties of the container in which the hello-pelion-connect application will run on the device. The OCI container configuration is located in the `src_bundle` folder.    


## Create hello-pelion-connect IPK

The hello-pelion-connect application is created inside the Docker container. This Docker container contains all the necessary tools that are required for the application creation (and therefore the required tools are not prerequisites of the user PC). 

The Docker container is built and run by the main script `container_create_hello_pelion_connect.sh`. This script is the main script which is used for the hello-pelion-connect application IPK creation. The resulting IPK contains the containerized hello-pelion-connect application. 

During the build, the Docker container runs `container_entry_point.sh` which sets the user id and user name to those of the user.
After that, `create_sample_application.sh` creates IPK package using OPKG tools.

To create hello-pelion-connect application navigate to the `<mbl-core>/tutorials/hello_pelion_connect` folder, and run ```container_create_hello_pelion_connect.sh```.

Creation script product is the IPK package `ipk/hello-pelion-connect_1.0_armv7vet2hf-neon.ipk` containing containerized hello-pelion-connect application.

To clean the previously created artifacts run
```
container_create_hello_pelion_connect.sh clean
```

## Install hello-pelion-connect IPK

Install the application IPK on the device, using one of the following ways:

 * [Send the application using over-the-air firmware update with the Pelion Device Management][over-the-air-firmware-update].
 * [Flash the application over USB with mbl-cli][mbl-cli-flash].

## Run hello-pelion-connect in OCI container
After the application IPK has been successfully installed on the device, expect to have the `/home/app/hello-pelion-connect/` folder on the device. This folder contains containerized (in OCI container) hello-pelion-connect application.

To run hello-pelion-connect application inside the OCi container, navigate to the `/home/app/hello-pelion-connect/` folder and run
```runc run <oci_container_id>```
-  `<oci_container_id>` is an arbitrary value, for example 123

## Analyzing result
### Status success 
On the successful operation, expect log lines informing about the progress of the operation being printed to the STDOUT without any error messages. On the successful execution, RegisterResources method result is the registration of the application resources in the Pelion Device Management portal. [This section][hello-pelion-connect-readme-analyzing-result] explains how to verify resource registration in the Pelion portal. 

### Status failure 
If during the application execution any error has occurred, execution will be aborted and the error will be printed to the device's standard error output. 

[over-the-air-firmware-update]: https://os.mbed.com/docs/mbed-linux-os/v0.5/getting-started/tutorial-updating-mbl-devices-and-applications.html
[mbl-cli-flash]: https://os.mbed.com/docs/mbed-linux-os/v0.5/tools/device-update.html#update-an-application
[install-docker]: https://docs.docker.com/install/linux/docker-ce/ubuntu
[hello-pelion-connect-readme]: ./source/README.md

[hello-pelion-connect-readme-analyzing-result]: ./source/README.md#analyzing-result