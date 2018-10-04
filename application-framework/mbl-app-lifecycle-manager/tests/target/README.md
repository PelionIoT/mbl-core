# Instruction for using mbl-app-lifecycle-manager_tests.py

1. Build pytest on HOST using the following commands:
   bitbake pytest-mbl-testing
   bitbake package-index

2. Make sure IP connectivity exists between HOST and target

3. Configure opkg on target to point to the IPK feed on the HOST (e.g. by modifying /etc/opkg/opkg.conf)

4. Install pytest on target using the following commands:
   opkg update
   opkg install pytest-mbl-testing

5. Push mbl-app-lifecycle-manager_tests.py to /mnt/cache/ on target.

6. Push app-lifecycle-manager-test-package_1.0_armv7vet2hf-neon.ipk to /mnt/cache/ directory on target.
   IPK file can be downloaded from https://github.com/ARMmbed/mbl-scratch/tree/master/mbl-app-lifecycle-manager/tests/data
   Commit hash: 64c21a344c4eebd46d7308338d71ba510dc20323

7. Run pytest on target run using following commands:
   cd /home/app/usr/bin
   ./python3 ./pytest.python3-pytest -s /mnt/cache/mbl-app-lifecycle-manager_tests.py

## Notes
1. When /mnt/cache/ directory name will be change to /scratch/ - Need to update tests path and push the test script to /scratch/
