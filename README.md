# Frame

3D engine example it use OpenGL and soon it will be using Vulkan. It take 
models in obj forms and images that can be read by stb (jpg, png, hdr,...).

![SceneRendering](https://github.com/anirul/ShaderGL/raw/master/Sample/SceneRendering.png "A Scene rendering made with ShaderGL.")

## Install

You will need vcpkg to use the cmake that is in the root of the repository.
If you install it you can install it in the ```~/Documents/GitHub/vcpkg``` 
or go to ```~/Documents/GitHub``` and clone it by using 
```git clone https://github.com/microsoft/vcpkg.git```. If you do so you won't
have to follow the vcpkg part of this tutorial, skip ahead to the cmake part.

### vcpgk

You can have a look at vcpkg in [here](https://github.com/microsoft/vcpkg) and
install it:

```cmd
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
```

### cmake

Install cmake from [here](https://cmake.org/). Check if you can execute it at
the command line and if not use the GUI version.

```cmd
mkdir Build/
cd Build
cmake .. -DCMAKE_TOOLCHAIN_FILE="[path_to_vcpkg]\scripts\buildsystems\vcpkg.cmake"
```
or if you have installed vcpkg in the ```~/Documents/GitHub/``` directory you 
can simply:

```cmd
mkdir Build
cd Build
cmake ..
```

### Visual Studio

Open the project with Visual studio 2019 or 2022.

### makefiles

You can just use the ```make``` command.

## Use

You have 6 program that can be used:

### ShaderGLTest

This is a test for the functionality of the ShaderGLLib.

### JapaneseFlag

![JapaneseFlag](https://github.com/anirul/ShaderGL/raw/master/Sample/JapaneseFlag.png "A rendering of the Japanese flag using shaders.")

This is a very bare bone Shader example that will open a window and display
the Japanese flag (using shaders).

### RayMarching

![RayMarching](https://github.com/anirul/ShaderGL/raw/master/Sample/RayMarching.png "A rendering of a sphere on a plane using raymaching shaders.")

This is a simple ray marching example, it will draw a sphere and compute the
shading and the shadow.

### Simple

![Simple](https://github.com/anirul/ShaderGL/raw/master/Sample/Simple.png "A rendering of an apple floating in the coulds.")

This will draw a cube map and an apple (simple just albedo).

### SceneRendering

This is a complete scene rendering. It loads ```obj``` from the ```Asset/```
directory directly into the 3D software. This render the scene using
Physically Based Rendering, the parameters are fetch from ```mtl``` files.
It also add Screen Space Ambient Occlusion and Bloom effect before rendering.

![SceneRendering](https://github.com/anirul/ShaderGL/raw/master/Sample/SceneRendering.png "A Scene rendering made with ShaderGL.")
