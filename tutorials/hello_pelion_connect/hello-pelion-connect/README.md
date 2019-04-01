# Pelion Connect hello-world application

The Python Pelion Connect hello-world application demonstrates different Pelion Connect D-Bus API methods invocation. Demonstrated methods:
1. RegisterResources - Pelion Connect D-Bus API method which registers LwM2M resources provided by the application in the Device Management Portal.

## Prerequisites   
1. Make sure the device has an Internet connection.

1. Make sure you have active Device Management Portal account. For more information see [this page][account-management].

1. Make sure your device was provisioned with developer certificate. For more information see [this page][provisioning-process].

1. Make sure your device has Update Resources file. For more information see [this page][update-resources-file].

## Setting up Pelion Connect hello-world application
The Pelion Connect hello-world application runs inside an OCI container. The description, how the application can be containerized please see in [the following readme][../README.md]. 

## Setting up Pelion Connect hello-world application for debug
The Pelion Connect hello-world application can be easily debugged if it runs in the Python virtual environment. To set up the Python virtual environment, perform the following steps:

1. Copy application subtree `<mbl-core>/tutorials/hello_pelion_connect`
   to the device under the `/scratch` partition.

1. Set the current directory as follows:
   ```shell
   cd /scratch/hello_pelion_connect/hello-pelion-connect/
   ```
   
1. Create a Python virtual environment:
   ```shell
   python3 -m venv my_venv
   ```

1. Activate virtual environment:
   ```shell
   source ./my_venv/bin/activate
   ```

1. Upgrade pip to the latest version
    ```
    pip install --upgrade pip setuptools
    ```
    
1. Install all the necessary packages in the virtual environment:
   ```shell
   pip install vext vext.gi pydbus
   pip install . --upgrade
   ```
1. Run Pelion Connect hello-world application
   ```shell
   hello_pelion_connect
   ```

## Analyzing result

### Status success 
On the successful operation, expect that resources provided in the application resources definition file are appeared registered in the Pelion Device Management portal. Registered resources for the particular device can be viewed in the "Devices" -> "Device details" -> "Resources" tab in the Pelion Device Management portal.

### Status failure 
If during the application execution any error has occurred, an execution will be aborted and the error will be printed to the device's standard error output. 

[account-management]: https://cloud.mbed.com/docs/current/account-management/users.html
[provisioning-process]: https://cloud.mbed.com/docs/v1.2/provisioning-process/provisioning-development.html
[update-resources-file]: https://os.mbed.com/docs/mbed-linux-os/v0.5/getting-started/preparing-device-management-sources.html#creating-an-update-resources-file
