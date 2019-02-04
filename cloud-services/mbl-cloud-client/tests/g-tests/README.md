# Google Tests for MBL Cloud Client

Google Tests provides framework and tests for MBL Cloud Client components such as LWM2M Resource Broker.


## Build

In order to build tests executable use ```ccrb-test``` recipe:
```
bitbake ccrb-test
```

```gtest-mbl-cloud-client``` executable will be located under ```deploy-ccrb-test/gtest/```.


## Install

Install tests on a device using scp or mbl-cli. Tests could be installed to any
writable location, for example to the ```scratch``` partition:
```
mbl-cli put gtest-mbl-cloud-client /scratch/gtest-mbl-cloud-client
```

## Run

On device change tests permissions to executable and run tests:
```
cd /scratch
chmod +x gtest-mbl-cloud-client
./gtest-mbl-cloud-client
```

To see line flags controlling the behavior of Google Test use ```--help```
```
./gtest-mbl-cloud-client --help
```

Google Tests could generate an XML report in the given directory or with the given file name:
```
./gtest-mbl-cloud-client --gtest_output=xml[:DIRECTORY_PATH/|:FILE_PATH]
```

```FILE_PATH``` default is ```test_details.xml```.

For more information regarding Google Tests refer [a quick introduction to the Google C++ Testing Framework](https://www.ibm.com/developerworks/aix/library/au-googletestingframework.htmld).


## Add tests

Tests could be added to an existing or a new file. If sub-directory with tests is added, the recipe should be updated to include the new path.
