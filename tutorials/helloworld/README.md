Build an image from a Dockerfile    
================================    
    
1. On Linux Ubuntu PC install Docker CE as described here:        
    
   https://docs.docker.com/install/linux/docker-ce/ubuntu/      
    
2. cd mbl-core/tutorials/helloworld    
       
3. Build dockcross (Cross compiling toolchains in Docker images) for arm-v6 platform.       
   The parant image is defined in Dockerfile: dockcross/linux-armv6.      
   docker build command prepares cross-compilation environment for building       
   user sample application inside the container.      
       
   docker build -t linux-armv6:latest ./cc-env/    
       
       
Build "helloworld" application with cross compiler    
==================================================    
  
Build produces IPK file:    
  
   * For release: ./release/ipk/user-sample-app-package_1.0_armv7vet2hf-neon.ipk  
   * For debug: ./debug/ipk/user-sample-app-package_1.0_armv7vet2hf-neon.ipk  
    
   * build-armv6 make all            // build both release and debug      
   * build-armv6 make release   // build release      
   * build-armv6 make debug     // build debug      
   * build-armv6 make clean      // clean the build      
     
       
Run on target    
=============    
    
1. Copy user-sample-app-package_1.0_armv7vet2hf-neon.ipk package to the device.    
    
2. After the device boot, copy user-sample-app-package_1.0_armv7vet2hf-neon.ipk     
   package under /scratch.    
    
3. Install the ipk using mbl-app-manager script:    
       
   cd /usr/bin      
   mbl-app-manager -i /scratch/user-sample-app-package_1.0_armv7vet2hf-neon.ipk -v    
    
4. Run container with sample application:    
    
   mbl-app-lifecycle-manager -r &lt;CONTAINER_ID&gt; -a user-sample-app-package      
       
   Type "runc list" to list all containers with there statuses.       
       
5. Also, user sample application will run after rebooting the device.
