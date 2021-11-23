# 3d-render-examples
Examples of 3D rendering applications based on Vulkan and OpenGL APIs.
## Build
This project has two submodules that need to be loaded before build. Also, you need to have the following packages to be installed first:
* VulkanSDK - https://vulkan.lunarg.com/sdk/home
* GLEW - https://github.com/nigels-com/glew/releases/download/glew-2.2.0/glew-2.2.0.zip

Then use cmake to generate project build files:

    mkdir -p build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release/Debug

Build tested with
* GNU/Linux (Ubuntu 20.04)
* MinGW 9.0
* MSVS 2019

## Run examples
Examples could be run in Vulkan or OpenGL modes.

    ./basic -v      # Vulkan Mode (default)
    ./basic -ogl    # OpenGL mode

## Current issues
* Choosing of device is not supported, and it picks the first enumerated device
* The only more or less stable build is for MinGW 9.0 toolchain.
* Codebase need major refactoring
* only few examples
* majority of Vulkan and OGL features are yet to be covered