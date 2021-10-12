# Basic graphics pipeline sample using Vulkan API
This project provides basic graphics applications samples:
* Forward rendering of a 3d scene in Hydra Renderer XML format ([HydraAPI](https://github.com/Ray-Tracing-Systems/HydraAPI), [Hydra renderer](http://www.raytracing.ru/)) located [here](https://github.com/msu-graphics-group/vk_graphics_basic/tree/main/src/samples/simpleforward). This sample has two renderers, which are selected directly in [code](https://github.com/msu-graphics-group/vk_graphics_basic/blob/main/src/samples/simpleforward/main.cpp) (search for *CreateRender*):
  * *SIMPLE_FORWARD* renders scene in diffuse material
  * *SIMPLE_TEXTURE* renders scene in diffuse textured material
* Shadow map sample located in [shadowmap](https://github.com/msu-graphics-group/vk_graphics_basic/tree/main/src/samples/shadowmap)
* Full screen quad render located in [quad2d](https://github.com/msu-graphics-group/vk_graphics_basic/tree/main/src/samples/quad2d)

You can also take a look at [Chimera project](https://gitlab.com/vsan/chimera) which served as a base for these samples and implements other example renders
including various approaches to using hardware accelerated ray tracing.

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

Executable will be built in *bin* subdirectory - *vk_graphics_basic/bin/renderer*

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
git clone --depth 1 https://github.com/zeux/volk.git
```

#### imgui:
```
cd vk_graphics_basic/external
git clone --depth 1 https://github.com/ocornut/imgui.git
```

#### vkutils:
```
cd vk_graphics_basic/external
git clone --depth 1 https://github.com/msu-graphics-group/vk-utils.git vkutils
```

### Single header libraries
This project also uses LiteMath and stb_image which are included in this repo.

https://github.com/msu-graphics-group/LiteMath

https://github.com/nothings/stb

