
Build an image from a Dockerfile
--------------------------------

1. On Linux Ubuntu install Docker CE from following path: 

   https://docs.docker.com/install/linux/docker-ce/ubuntu/

2. cd mbl-core/tutorials/helloworld
   
3. Build an image from a Dockerfile located under cc-env directory. 
   The parant image is defined in Dockerfile: dockcross/linux-armv6. 
   docker build command prepares cross-compilation environment for building 
   user sample application inside the container.
   
   docker build -t linux-armv6:latest ./cc-env/
   
   -t option tags the resulting image.
   
   
Build "helloworld" application with cross compiler
--------------------------------------------------

   build-armv6 make all       // build both release and debug
   build-armv6 make release   // build release
   build-armv6 make debug     // build debug
   build-armv6 make clean     // clean the build
   
   
Run on target
-------------

1. Copy user-sample-app-package_1.0_armv7vet2hf-neon.ipk package to the device.

2. After the device boot, copy user-sample-app-package_1.0_armv7vet2hf-neon.ipk 
   package under /usr/bin.

3. Install the ipk using mbl-app-manager script:
   
   cd /usr/bin
   mbl-app-manager -i user-sample-app-package_1.0_armv7vet2hf-neon.ipk -v

4. Run container with sample application:

   mbl-app-lifecycle-manager -r <CONTAINER_ID> -a user-sample-app-package
   
   Type "runc list" to list all containers with there statuses.  
   
5. Reboot the device and make sure the user sample application will run after the boot. 
