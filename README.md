# Basic graphics pipeline sample using Vulkan API
This project implements basic forward rendering on a 3d scene in Hydra Renderer XML format (https://github.com/Ray-Tracing-Systems/HydraAPI, http://www.raytracing.ru/)

This project is a simplified part of https://gitlab.com/vsan/chimera which implements other example rendering types 
(including various approaches to use hardware accelerated ray tracing).

## Build

Build as you would any other CMake-based project. For example on Linux:
```
cd vk_graphics_basic
mkdir build
cd build
cmake ..
make -j8
```

On Windows you can directly open CMakeLists.txt as a project in recent versions of Visual Studio. Mingw-w64 building is also supported.

Executable will be build in *bin* subdirectory - *vk_graphics_basic/bin/renderer*

## Dependencies
### Vulkan 
SDK can be downloaded from https://vulkan.lunarg.com/

### GLFW 
https://www.glfw.org/

Linux - you can install it as a package on most systems, for example: 
```
sudo apt-get install libglfw3-dev
```

Windows - you don't need to do anything, binaries and headers are included in this repo (see external/glfw directory).

### Additional dependencies 
Can be installed by running provided clone_dependencies.bat script. So, if you clone this repo into vk_graphics_basic directory:

```
cd vk_graphics_basic
clone_dependencies.bat
```

Or you can get them manually:

#### volk:
```
cd vk_graphics_basic/external
git clone https://github.com/zeux/volk.git
```

#### vkutils:
```
cd vk_graphics_basic/external
git clone https://gitlab.com/vsan/vkutils.git
```

### Single header libraries
This project also uses  (LiteMath, tinyobj, stb_image) which are included in this repo.

https://github.com/msu-graphics-group/LiteMath

https://github.com/tinyobjloader/tinyobjloader

https://github.com/nothings/stb

