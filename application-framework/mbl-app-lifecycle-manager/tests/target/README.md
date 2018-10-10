# Instructions for using mbl-app-lifecycle-manager_tests.py

1. Build pytest on the host machine using the following commands:
   bitbake pytest-mbl-testing
   bitbake package-index

2. Make sure IP connectivity exists between the host machine and the target device.

3. Configure opkg on the target device to point to the IPK feed on the the host machine (e.g. by modifying /etc/opkg/opkg.conf)

4. Install pytest on the target device using the following commands:
   opkg update
   opkg install pytest-mbl-testing

5. Push mbl-app-lifecycle-manager_tests.py to /mnt/cache/ on the target device.

6. Push app-lifecycle-manager-test-package_1.0_armv7vet2hf-neon.ipk to the /mnt/cache/ directory on the target device.
   The IPK file can be downloaded from https://github.com/ARMmbed/mbl-scratch/tree/64c21a344c4eebd46d7308338d71ba510dc20323/mbl-app-lifecycle-manager/tests/data

7. Run pytest on the target device run using following commands:
   cd /home/app/usr/bin
   ./python3 ./pytest.python3-pytest -s /mnt/cache/mbl-app-lifecycle-manager_tests.py
