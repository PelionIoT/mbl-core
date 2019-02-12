# MBL applications Cloud Client connectivity over DBus test

The MBL application runs in a virtual environment.

To set up the virtual environment, perform the following steps:

1. Copy test application subtree `mbl-core/application-framework/tests/mbl-app-cloud-connect`
   to the device under the `/scratch` partition.
   
1. Make sure the device has internet connection 

1. Enter mbl-app-cloud-connect directory
   ```shell
    cd /scratch/mbl-app-cloud-connect
   ```
   
1. Create virtual environment:
   ```shell
   python3 -m venv my_venv
   ```

1. Activate virtual environment:
   ```shell
   source ./my_venv/bin/activate
   ```

1. Upgrade pip to latest version
    ```
    pip install --upgrade pip
    ```
    
1. Install all the necessary packages in the virtual environment:
   ```shell
   pip install -r requirements.txt
   pip install . --upgrade
   ```

Run pytests:
   ```shell
   pytest
   ```

Exit virtual environment:
   ```shell
   deactivate
   ```
