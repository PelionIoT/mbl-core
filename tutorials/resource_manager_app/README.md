## Purpose

Framework for creation python LWM2M resource manager sample application.

## Create LWM2M resource manager sample application

To create LWM2M resource manager sample application run
```
container_create_lwm2m_resource_manager_sample_app.sh
```

The result is the IPK package ```ipk/lwm2m-resource-manager-sample-app_1.0_armv7vet2hf-neon.ipk``` 
containing LWM2M resource manager sample application.

To clean the previously created artifacts run
```
container_create_lwm2m_resource_manager_sample_app.sh clean
```

## Install and run

Install and run IPK package containing LWM2M resource manager sample application using Pelion.

## Structure

```mbl-core/tutorials/resource_manager_app``` directory composed of scripts and sub-directory structure
which introduce framework for creation python LWM2M resource manager sample application.

The main script creating LWM2M resource manager sample application is ```container_create_lwm2m_resource_manager_sample_app.sh```.
This script creates an IPK package containing a python application sources located under ```src```.
The resulting IPK package will be located under the ```ipk``` directory.

## How it works

The LWM2M resource manager sample application is created inside a Docker container. This way all the necessary tools
are provided inside the container and are not prerequisites of the user PC. 

The Docker container is built and run by the main script ```container_create_lwm2m_resource_manager_sample_app.sh```
The container run  ```container_entry_point.sh``` which sets the user id and user name to those of the user running the main script.
After that ```create_sample_application.sh``` do IPK package using OPKG tools.
```Dockerfile``` could be found under ```cc-env/```. 

After the application is installed on the device, run it inside the container. The container configuration could be found 
at ```src_bundle/config.json```.
