# Google Test (gtest) Tests for MBL Cloud Client

This folder contains tests for the MBL Cloud Client components, such as the LWM2M Resource Broker.
Building the executable should be done using meta-mbl/recipes-connectivity/mbl-cloud-client/mbl-cloud-client-gtest.bb


## Build

In order to build tests executable use ```mbl-cloud-client-gtest.bb``` recipe:
```
bitbake mbl-cloud-client-gtest
```

```mbl-cloud-client-gtest``` executable will be located under ```${DEPLOYDIR}/${TARGET_OS}/bin```.


## Install

Install tests on a device using scp or mbl-cli. Tests could be installed to any
writable location, for example to the ```scratch``` partition:
```
mbl-cli put mbl-cloud-client-gtest /scratch/mbl-cloud-client-gtest
```

## Run

On device change tests permissions to executable and run tests:
```
cd /scratch
chmod +x mbl-cloud-client-gtest
./mbl-cloud-client-gtest
```

To see line flags controlling the behavior of Google Test use ```--help```
```
./mbl-cloud-client-gtest --help
```

Google Tests could generate an XML report in the given directory or with the given file name:
```
./mbl-cloud-client-gtest --gtest_output=xml[:DIRECTORY_PATH/|:FILE_PATH]
```

```FILE_PATH``` default is ```test_details.xml```.

For more information regarding Google Tests refer [a quick introduction to the Google C++ Testing Framework](https://www.ibm.com/developerworks/aix/library/au-googletestingframework.html).


## Add tests

Tests could be added to an existing or a new file. If sub-directory with tests is added, the recipe should be updated to include the new path.
