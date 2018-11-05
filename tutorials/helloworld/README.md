Build an image from a Dockerfile          
================================          
          
1. On Linux Ubuntu PC install Docker CE as described here:              
          
    https://docs.docker.com/install/linux/docker-ce/ubuntu/            
          
1. cd mbl-core/tutorials/helloworld          
             
1. Build dockcross (Cross compiling toolchains in Docker images) for arm-v6 platform.             
   The parant image is defined in Dockerfile: dockcross/linux-armv6.            
   docker build command prepares cross-compilation environment for building             
   user sample application inside the container.            
             
   docker build -t linux-armv6:latest ./cc-env/          
             
             
Build "helloworld" application with cross compiler          
==================================================          
        
Build produces IPK file:          
        
   * For release: ./release/ipk/user-sample-app-package_1.0_armv7vet2hf-neon.ipk        
   * For debug: ./debug/ipk/user-sample-app-package_1.0_armv7vet2hf-neon.ipk        
          
   * build-armv6 make all       // build both release and debug            
   * build-armv6 make release   // build release            
   * build-armv6 make debug     // build debug            
   * build-armv6 make clean     // clean the build            
           
             
Run on target          
=============          
          
1. Copy user-sample-app-package_1.0_armv7vet2hf-neon.ipk package to the device.          
          
1. After the device boot, copy user-sample-app-package_1.0_armv7vet2hf-neon.ipk           
   package under /scratch.          
          
1. Install the and run ipk using app_update_manager.py script. (TODO: currently      
   master is broken, after the fix add explanation how to use app_update_manager.py      
   script.      
           
1. Also, user sample application will run after rebooting the device.

