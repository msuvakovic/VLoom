# VLoom

## Brief Description

This project stands for "VibeLoom" aka "This is 95% vibe coded with claude code with C++ like a Loom". I also built this primarily with GLFW GLAD OpenGL and a few other open source. I also fed instructions into claude code how to manage and organize the code (RenderDevice, Mesh and Sceneobject splits, caching devices, etc)

# Current Features
- simple text file load and save scenes
- normal mapping
- point light that folows player
- FPS-like movement (wasd, jumping, etc)
- Mouse look
- affine texture/PS1 style  wrapping(albiet a bit wonky at the moment)
- caching meshes and textures to avoid redundant loads
- editing of objects within scenes (rotation, position and scale)
- Triplanar texture mapping
- Windows Cross compile support
- Imgui support 
- FBX loading via Assimp

# Build instructions installation
Note: these instructions are intended to be run on linux. Your mileage may vary if you try to build on windows


## Linux:
### Linux build tools
```bash
# Debian/Ubuntu
sudo apt install cmake git build-essential libgl1-mesa-dev

# Arch
sudo pacman -S cmake git base-devel mesa
```

### Linux Building
```bash
cmake -B build
cmake --build build -j$(nproc)
```

## Windows cross-compile (from linux)
### Windows cross-compile tools 
```bash
# Debian/Ubuntu
sudo apt install cmake git build-essential mingw-w64

# Arch
sudo pacman -S cmake git base-devel mingw-w64-gcc
```
### Building for windows on linux
```bash
cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw.cmake
cmake --build build-win -j$(nproc)
```

### Windows zip (important)
```bash
cd build-win && zip -r ../VLoom-windows.zip cube_renderer.exe bricks_color.jpg bricks_normal.jpg ground.fbx ramp1.fbx railing.fbx shaders/ ../THIRD_PARTY_LICENSES.txt
```



