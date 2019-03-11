# MBL Pelion Connect hello-world application

The Pelion Connect hello-world application runs in a virtual environment.

To set up the virtual environment, perform the following steps:

1. Copy application subtree `/mbl-core/tutorials/hello_pelion_connect`
   to the device under the `/scratch` partition.
   
1. Make sure the device has internet connection 

1. Set the current directory as follows:
   ```shell
    cd /scratch/hello_pelion_connect
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

Run application:
   ```shell
   hello_pelion_connect
   ```

Exit virtual environment:
   ```shell
   deactivate
   ```
   

