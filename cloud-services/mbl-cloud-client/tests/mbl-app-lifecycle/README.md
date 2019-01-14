# MBL applications Cloud Client connectivity.

MBL application is running inside virtual environment.

In order to setup virtual environment perform next steps:

1. Copy test application subtree `/mbl-core/cloud-services/mbl-cloud-client/tests/mbl-app-lifecycle`
   to the device under the `/scratch` partition.

1. Create virtual environment:
   ```shell
   python3 -m venv my_venv
   ```

1. Activate virtual environment:
   ```shell
   source ./my_venv/bin/activate
   ```

1. Install all the nessasary packages inside virtual environment:
   ```shell
   pip install -r requirements.txt
   pip install . --upgrade
   ```

Create session D-Bus (temporary solution, untill the D-Bus deamon will run 
automatically on the device boot):
   ```shell
   dbus-launch --config-file ./ipc.conf
   ```

Following is a sample output of session D-Bus creation:
   ```shell
   DBUS_SESSION_BUS_ADDRESS=unix:path=/scratch/mbl-app-lifecycle/mbl/app_lifecycle/server_client_socket,guid=a7b20c57341193b6981973785c34562c
   DBUS_SESSION_BUS_PID=23434
   ```

Run pytests:
   ```shell
   DBUS_SESSION_BUS_ADDRESS=unix:path=./mbl/app_lifecycle/server_client_socket DISPLAY=0 pytest
   ```

Exit virtual environment:
   ```shell
   deactivate
   ```
