# Instructions for using mbl-app-lifecycle-manager_tests.py

1. Make sure IP connectivity exists between the host machine and the target device.

2. Configure opkg on the target device to point to the IPK feed on the the host machine

3. Install pytest on the target device

4. Push mbl-app-lifecycle-manager_tests.py to /mnt/cache/ on the target device.

5. Push app-lifecycle-manager-test-package_1.0_armv7vet2hf-neon.ipk to the /mnt/cache/ directory on the target device.
   The IPK file can be downloaded from https://github.com/ARMmbed/mbl-scratch/tree/64c21a344c4eebd46d7308338d71ba510dc20323/mbl-app-lifecycle-manager/tests/data

6. Run pytest on the target device using following commands:
   python3 <pytests path>/pytest.python3-pytest -s /mnt/cache/mbl-app-lifecycle-manager_tests.py

* Note - Above instructions are provisional way of running Pytest.
