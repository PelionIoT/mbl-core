# Instruction for using mbl-app-lifecycle-manager_tests.py

1. Build pytest on HOST using the following commands:
   bitbake pytest-mbl-testing
   bitbake package-index

2. Install pytest on target using the following commands:
   opkg update
   opkg install pytest-mbl-testing

3. Push mbl-app-lifecycle-manager_tests.py to /usr/lib/python3.5/site-packages/ on target.

4. Push app-lifecycle-manager-test-package_1.0_armv7vet2hf-neon.ipk to root directory on target.
   IPK file can be downloaded from https://github.com/ARMmbed/mbl-scratch/tree/master/mbl-app-lifecycle-manager/tests/data

5. Run pytest on target run using following commands:
   cd /home/app/usr/bin
   ./python3 ./pytest.python3-pytest -s /usr/lib/python3.5/site-packages/mbl-app-lifecycle-manager_tests.py
