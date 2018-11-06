Build an image from a Dockerfile            
================================            
            
1. On Linux Ubuntu PC install Docker CE as described here:                
            
    https://docs.docker.com/install/linux/docker-ce/ubuntu/              
            
1. cd mbl-core/tutorials/helloworld            
               
1. Build dockcross (Cross compiling toolchains in Docker images).               
   The parant image is defined in Dockerfile: dockcross/linux-armv7.              
   docker build command prepares cross-compilation environment for building               
   user sample application inside the container.              
               
   docker build -t linux-armv7:latest ./cc-env/    
     
1. The image does not need to be run manually. Instead, there is a helper    
   script to execute build commands on source code existing on the local host    
   filesystem. This script is bundled with the image.  
  
   To install the helper script, run the image with no arguments, and redirect    
   the output to a file:  
  
   docker run --rm linux-armv7 &gt; build-armv7  
   chmod +x build-armv7  
   sudo mv build-armv7 /usr/local/bin  
  
              
Build "helloworld" application with cross compiler            
==================================================            
          
Build produces IPK file:            
          
   * For release: ./release/ipk/user-sample-app-package_1.0_armv7vet2hf-neon.ipk          
   * For debug: ./debug/ipk/user-sample-app-package_1.0_armv7vet2hf-neon.ipk      
     
In order to build, invoke make toolchain command:        
            
   * build-armv7 make all       // build both release and debug              
   * build-armv7 make release   // build release              
   * build-armv7 make debug     // build debug              
   * build-armv7 make clean     // clean the build              
             
               
Run on target            
=============            
            
1. Create user-sample-app-package_1.0_armv7vet2hf-neon.tar:  
  
   tar -cvf user-sample-app-package_1.0_armv7vet2hf-neon.tar user-sample-app-package_1.0_armv7vet2hf-neon.ipk  
            
1. Copy user-sample-app-package_1.0_armv7vet2hf-neon.tar to the device under /scratch.            
            
1. Extract, install the and run ipk using app_update_manager.py script:    
  
   mbl-app-update-manager -i /scratch/user-sample-app-package_1.0_armv7vet2hf-neon.tar  
             
1. Also, user sample application will run after rebooting the device.

