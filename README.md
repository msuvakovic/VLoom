# VLoom

First-person OpenGL 3.3 renderer. Walk on a floating island, jump, and explore with a point light that follows your eye position.

---

## Dependencies

All fetched automatically by CMake at configure time (no manual installs needed for the game itself):
- GLFW 3.4, GLM 1.0.1, GLAD (GL 3.3 core), Assimp v5.3.1, ImGui v1.91.8, stb

### Linux build tools
```bash
# Debian/Ubuntu
sudo apt install cmake git build-essential libgl1-mesa-dev

# Arch
sudo pacman -S cmake git base-devel mesa
```

### Windows cross-compile tools (from Linux)
```bash
# Debian/Ubuntu
sudo apt install cmake git build-essential mingw-w64

# Arch
sudo pacman -S cmake git base-devel mingw-w64-gcc
```

---

## Building

### Linux
```bash
cmake -B build
cmake --build build -j$(nproc)
```

Run:
```bash
# Integrated GPU
./build/cube_renderer

# NVIDIA discrete GPU (prime offload)
NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia ./build/cube_renderer
```

---

### Windows (cross-compiled from Linux with MinGW)
```bash
cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw.cmake
cmake --build build-win -j$(nproc)
```

Output: `build-win/cube_renderer.exe` + `build-win/libassimp.dll`

To run on a Windows machine, copy these files to the same folder:
```
cube_renderer.exe
libassimp.dll
shaders/          ← copy the whole folder
bricks_color.jpg
ground.fbx
ramp1.fbx
railing.fbx
```
> All of these are already generated inside `build-win/` by CMake — just copy that directory.

If the exe crashes immediately, it may need `libwinpthread-1.dll` from your MinGW install:
```bash
# Find and copy it
find /usr/x86_64-w64-mingw32 -name "libwinpthread-1.dll" 2>/dev/null
cp <path> build-win/
```
`libstdc++` and `libgcc` are statically linked so those DLLs are not needed.

---

## Adding new assets

1. Copy the FBX into `assets/Pieces/`
2. Add a `configure_file(...)` line to `CMakeLists.txt`
3. Re-run `cmake -B build` (or `cmake -B build-win ...`) to copy it to the build dir
4. Load it in `main.cpp` with `Mesh::load(exePath("yourfile.fbx").c_str())`

---

## Controls

| Key | Action |
|-----|--------|
| WASD | Move |
| Space | Jump |
| Mouse | Look (FPS) |
| Tab | Toggle cursor lock / open ImGui panel |
| F1 | Wireframe debug mode |
| Escape | Quit |

ImGui panel (Tab):
- **Retro Texture Warp** — toggle between triplanar (default) and affine PS1-style UV mapping
- **Scene Editor** — move/rotate/scale loaded objects in real time
