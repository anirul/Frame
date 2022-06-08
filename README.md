# Frame

3D engine example it use OpenGL and soon it will be using (Vulkan or DirectX 
12). It take models in obj forms and images that can be read by stb (jpg, png,
hdr,...).

![SceneRendering](https://github.com/anirul/Frame/raw/master/examples/scene_rendering.png "A Scene rendering made with ShaderGL.")

## Install

Now use *conan* for dependencies query and update, so you will need *python* 
and *conan* if you want to build the software. So first install 
[python](https://www.python.org/downloads/) version 3.x. Then install *conan* 
with *pip*.

```cmd
pip install conan
```

Go into the directory where you installed the software and create a new folder
for building and creating the environment.

```cmd
cd ~/Document/GitHub/Frame
mkdir Build
cd Build
```

Then use *conan* to install dependencies needed by *Frame*. Using the following
commands.

For Debug:
```cmd
conan install --build=missing -s build_type=Debug ..
```

For Release:
```cmd
conan install --build=missing -s build_type=Release ..
```

### cmake

Install *cmake* from [here](https://cmake.org/). Check if you can execute it at
the command line and if not use the GUI version.

Still in the *Build* directory you should use:
```cmd
cmake ..
```

This will generate your project file that you can specify your generator 
```-G``` to be either Visual Studio for windows or makefiles for Linux.

And then you can build either for Debug:
```cmd
cmake --build . --config=Debug
```

Or for Release:
```cmd
cmake --build . --config=Release
```

### Visual Studio

Open the project with Visual studio 2019 or 2022.

```cmd
./Frame.sln
```

### makefiles

You can just use the ```make``` command.

## Use

You have may programs that can be used:

### JapaneseFlag

![JapaneseFlag](https://github.com/anirul/Frame/raw/master/examples/japanese_flag.png "A rendering of the Japanese flag using shaders.")

This is a very bare bone Shader example that will open a window and display
the Japanese flag (using shaders).

### RayMarching

![RayMarching](https://github.com/anirul/Frame/raw/master/examples/ray_marching.png "A rendering of a sphere on a plane using raymaching shaders.")

This is a simple ray marching example, it will draw a sphere and compute the
shading and the shadow.

### Simple

![Simple](https://github.com/anirul/Frame/raw/master/examples/scene_simple.png "A rendering of an apple floating in the coulds.")

This will draw a cube map and an apple (simple just albedo).

### SceneRendering

This is a complete scene rendering. It loads ```obj``` from the ```Asset/```
directory directly into the 3D software. This render the scene using
Physically Based Rendering, the parameters are fetch from ```mtl``` files.
It also add Screen Space Ambient Occlusion and Bloom effect before rendering.

![SceneRendering](https://github.com/anirul/ShaderGL/raw/master/examples/SceneRendering.png "A Scene rendering made with ShaderGL.")
