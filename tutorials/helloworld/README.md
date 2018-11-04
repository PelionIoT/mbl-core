
Build an image from a Dockerfile
--------------------------------

1. Set the WORKDIR:

   export WORKDIR=/home/yelgor01/mbl

2. hello_word_app is a cross compilation libraty location:

   cd $WORKDIR/hello_word_app
   
3. Dockerfile installs inside the container opkg-utils and tar version 1.30
   Tar version 1.28 and latest is requiered by build-armv6 script.
   
4. Build an image from a Dockerfile located under cc-env directory.
   The docker build command builds Docker images from a Dockerfile and a “context”. 
   A build’s context is the set of files located in the specified PATH or URL. 
   
   docker build -t linux-armv6:latest ./cc-env/
   
   
Build "helloworld" application with cross compiler
--------------------------------------------------

   build-armv6 make all       // build both release and debug
   build-armv6 make release   // build release
   build-armv6 make debug     // build debug
   build-armv6 make clean     // clean the build
