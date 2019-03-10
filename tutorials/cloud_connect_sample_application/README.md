# MBL Cloud Client over DBus sample application

The Cloud Client sample application runs in a virtual environment.

To set up the virtual environment, perform the following steps:

1. Copy sample application subtree `/home/maxaxe01/work/mbl-core/tutorials/cloud_connect_sample_application`
   to the device under the `/scratch` partition.
   
1. Make sure the device has internet connection 

1. Set the current directory as follows:
   ```shell
    cd /scratch/cloud_connect_sample_application
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

Run sample application:
   ```shell
   cloud_connect_sample_application
   ```

Exit virtual environment:
   ```shell
   deactivate
   ```
   

