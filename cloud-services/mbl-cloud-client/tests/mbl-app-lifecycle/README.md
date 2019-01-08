# MBL applications Cloud Client connectivity.

MBL application is running inside virtual environment.

In order to setup virtual environment perform next steps:

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
pip install pip --upgrade
pip install vext
pip install vext.gi
pip install pydbus
pip install pytest
pip install . --upgrade
```

1. Create session D-Bus:
```shell
dbus-launch --config-file ./server_client_session.conf
DBUS_SESSION_BUS_ADDRESS=unix:path=/scratch/mbl-app-pydbus/mbl/app_lifecycle/server_client_socket,guid=a7b20c57341193b6981973785c34562c
DBUS_SESSION_BUS_PID=23434
```

1. Run pytests:
```shell
DBUS_SESSION_BUS_ADDRESS=unix:path=./mbl/app_lifecycle/server_client_socket DISPLAY=0 pytest
```

1. Exit virtual environment:
```shell
deactivate
```
