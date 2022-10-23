# Frame

3D engine example it use OpenGL and soon it will be using (Vulkan or DirectX
12). It take models in obj forms and images that can be read by stb (jpg, png,
hdr,...).

![SceneRendering](https://github.com/anirul/Frame/raw/master/examples/scene_rendering.png "A Scene rendering made with ShaderGL.")

## Building - New Scripts

As the project is using **conan** on top of CMake as its build gpu, the following commands need to be ran in order to build everything. The dependencies are automatically downloaded and managed by conan. Note that the unit tests of the library are ran automatically on every build.

**Make sure that your `volaio` environment is active.**

- First of all, the different command line arguments expected by the script can be printed easily.

```shell
python build.py -h
```

- It is possible to build the documentation:

```shell
python build.py docs
```

- It is possible to remove the files and folders generated previously:

```shell
python build.py clean
```

- It is also possible to select a build type (release or debug):

```shell
# Builds the gpu library in release mode only.
python build.py configure --type release
python build.py build
```

- It is possible to run the unit tests by running the following commands:

```shell
# Builds the shared lvv-gpu library (dll on windows).
python build.py --type release
python build.py build
python build.py test
```

- The examples can be enabled as well:

```shell
# Builds the gpu library with the examples.
python build.py configure --type release
python build.py build
```

- By default libraries are being built statically. A shared build can be enabled:

```shell
# Builds the shared lvv-gpu library (dll on windows).
python build.py --lib shared
python build.py build
```

## Install (old approach)

Now use _conan_ for dependencies query and update, so you will need _python_
and _conan_ if you want to build the software. So first install
[python](https://www.python.org/downloads/) version 3.x. Then install _conan_
with _pip_.

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

Then use _conan_ to install dependencies needed by _Frame_. Using the following
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

Install _cmake_ from [here](https://cmake.org/). Check if you can execute it at
the command line and if not use the GUI version.

Still in the _Build_ directory you should use:

```cmd
cmake ..
```

This will generate your project file that you can specify your generator
`-G` to be either Visual Studio for windows or makefiles for Linux.

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

You can just use the `make` command.

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

This is a complete scene rendering. It loads `obj` from the `Asset/`
directory directly into the 3D software. This render the scene using
Physically Based Rendering, the parameters are fetch from `mtl` files.
It also add Screen Space Ambient Occlusion and Bloom effect before rendering.

![SceneRendering](https://github.com/anirul/Frame/raw/master/examples/scene_rendering.png "A Scene rendering made with ShaderGL.")
