  # VLoom — Project Memory

  ## What this is
  A first-person OpenGL 3.3 renderer/game called **VLoom**. The player walks on a floating island, can jump, and is lit by a point light that follows them. Built incrementally: cube → textured ground → FBX loading → FPS controller → modular refactor.

  ## Build
  ```bash
  cmake -B build && cmake --build build -j$(nproc)
  NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia ./build/cube_renderer
  ```
  Assets and shaders are copied to the build dir by CMake (`configure_file ... COPYONLY`). Re-run CMake after adding new assets.

  ## File structure
  ```
  src/
    main.cpp          # Window/GL init, GLFW callbacks, ImGui, main loop
    Shader.hpp        # Shader class: fromFiles(), use(), setInt/Float/Vec3/Mat4()
    Mesh.hpp          # Mesh class: static load() via Assimp, draw(), AABB bounds, move-only
    Entity.hpp        # Entity: DEAD — no longer used. Platform converted to SceneObject. Safe to delete.
    Player.hpp        # Player: WASD+jump+gravity+collision, eyePosition(), update(const SceneObject&)
    Texture.hpp       # Texture class: static load() via stb_image, bind(unit), move-only RAII
    Camera.hpp        # Camera: azimuth/elevation, mouse look update(), lookDir(), resetMouse()
    RenderDevice.hpp  # RenderDevice: GL state manager; bindShader/bindTexture (cached), clear, draw
    MeshCache.hpp     # MeshCache: loads each FBX once, returns stable const Mesh& (std::map)
    TextureCache.hpp  # TextureCache: loads each texture path once, returns stable const Texture& (std::map)
    SceneObject.hpp   # SceneObject: meshPath, texPath + non-owning Mesh*/Texture* + TRS + transform; rebuildTransform(), worldAABB(), draw(RenderDevice&)
    SceneEditor.hpp   # SceneEditor: draw(vector<SceneObject>&) renders Tab UI per object
    SceneSerializer.hpp # SceneSerializer: save/load vector<SceneObject> to/from INI-style scene.txt
    stb_image.cpp     # STB_IMAGE_IMPLEMENTATION translation unit
  shaders/
    default.vert      # Phong vert: outputs vNormal, vFragPos, vModelPos, vUV (perspective), vUV_Affine (noperspective world-space XZ)
    default.frag      # Point light frag: triplanar OR affine UV (u_UseAffine), normal mapping (u_UseNormalMap), gl_FrontFacing correction, attenuation
  assets/
    Pieces/           # 79 binary FBX pieces (see Asset library section for full list)
    Textures/
      Bricks076A_1K-JPG (2)/
        Bricks076A_1K-JPG_Color.jpg        # → copied to build as bricks_color.jpg (active)
        Bricks076A_1K-JPG_AmbientOcclusion.jpg
        Bricks076A_1K-JPG_Displacement.jpg
        Bricks076A_1K-JPG_NormalGL.jpg     # → copied to build as bricks_normal.jpg (active, bound on unit 1)
        Bricks076A_1K-JPG_NormalDX.jpg
        Bricks076A_1K-JPG_Roughness.jpg
    texture.png       # Generic texture at assets root (unused in code)
    Sample 1-5/       # Sample scene FBX files (one per subfolder)
  grassblock.jpg      # Grassblock texture atlas (Minecraft-style, at project root, unused in code)
  CMakeLists.txt
  toolchain-mingw.cmake
  ```

  ## Dependencies (all via FetchContent)
  | Lib | Version | Notes |
  |-----|---------|-------|
  | GLFW | 3.4 | X11 only, Wayland off |
  | GLM | 1.0.1 | math |
  | GLAD | v0.1.36 | GL 3.3 core |
  | Assimp | v5.3.1 | FBX importer only (`ASSIMP_BUILD_FBX_IMPORTER ON`) |
  | ImGui | v1.91.8 | FetchContent_Populate (no CMakeLists), sources added via target_sources |
  | stb | master | header-only, stb_image.cpp provides the implementation TU |

  ## Key architecture decisions

  ### Shader
  - Source-string constructor + `static Shader::fromFiles(vertPath, fragPath)`
  - `readFile()` uses `std::ifstream` + `rdbuf()`; prints error and returns `""` if file missing

  ### Mesh
  - Move-only (no copy). `static Mesh::load(path)` factory.
  - Stores `boundsMin`/`boundsMax` in model-local space for AABB collision.
  - Vertex layout: `vec3 position, vec3 normal, vec2 uv` (stride = 8 floats), attribs 0, 1, 2.

  ### Entity
  - Holds `const Mesh&` (non-owning) + `glm::mat4 transform`.
  - `draw(RenderDevice&)` — sets `u_Model` and calls `rd.drawMesh()`.
  - `worldAABB()` transforms all 8 corners and returns `{wMin, wMax}`.
  - The Mesh must outlive its Entity.

  ### RenderDevice
  - Thin stateful wrapper around raw OpenGL. One instance lives in `main()`.
  - `bindShader(shader)` — skips `glUseProgram` if `shader.id` already active.
  - `bindTexture(tex, unit)` — skips `glActiveTexture + glBindTexture` if same `tex.id` on that unit. Tracks up to 8 units (`boundTextureIDs_[8]`).
  - `setInt/Float/Vec3/Mat4` — `glUniform*` forwarded to `boundShaderID_` via `glGetUniformLocation`.
  - `clear(r,g,b,a)`, `setViewport`, `setDepthTest`, `setPolygonMode` wrap the equivalent GL calls.
  - `drawMesh(mesh)` — calls `mesh.draw()` (exists as the canonical draw entry point for future batching).
  - Does NOT own any GL objects — purely state-tracks existing ones.

  ### Texture
  - Move-only RAII. `static Texture::load(path)` via stb_image (y-flip on).
  - `bind(unit)` wraps `glActiveTexture` + `glBindTexture`. `isValid()` checks `id != 0`.

  ### TextureCache
  - Exact same pattern as `MeshCache`: `std::map<string, Texture>`, returns stable `const Texture&`, deduplicates by path. `std::map` used so insertions never invalidate existing references.
  - `get(path, wrap = Repeat)` — loads on first call, returns cached reference on subsequent calls.
  - All SceneObject texture pointers source from the same cache — `bricks_color.jpg` is uploaded to the GPU only once.

  ### SceneObject
  - Non-owning `Mesh*` (into `MeshCache`) and non-owning `const Texture*` (into `TextureCache`). Also stores `meshPath` and `texPath` (basenames relative to exe) for serialization.
  - Holds `pos`, `rot` (Euler degrees), `scale`, and composed `glm::mat4 transform`.
  - `rebuildTransform()` composes translate → rotateY → rotateX → rotateZ → scale.
  - `draw(RenderDevice&)` — binds its texture via `rd.bindTexture()` (cache-aware), sets `uTileSize` + `u_Model`, calls `rd.drawMesh()`. Skips silently if `mesh.vao == 0`.
  - Move-only (copy deleted). Safe to store in `std::vector<SceneObject>`.
  - Adding a new scene prop: construct, set label/mesh/pos, call `rebuildTransform()`, `push_back(std::move(obj))`.

  ### SceneEditor
  - Minimal world-editing UI. `draw(vector<SceneObject>&)` iterates the vector directly — no stored state.
  - Renders nested collapsing headers (Position + Rotation + Scale) per object; uses `PushID/PopID` for unique widget IDs.
  - Calls `obj.rebuildTransform()` whenever any field changes.

  ### SceneSerializer
  - `save(filePath, objects)` / `load(filePath, meshCache, texCache, objects)` — header-only, no new dependencies.
  - INI-style format: `[object]` sections with `label`, `mesh`, `tex`, `pos`, `rot`, `scale`, `tile` fields.
  - Asset paths are basenames; resolved relative to the scene file's directory (= exe dir) on load.
  - Load clears `objects` and rebuilds from file. Ground platform is never saved (hardcoded in `main.cpp`).
  - Save/Load buttons in the Tab panel above the object list.

  ### Camera
  - `Camera` owns: `azimuth`, `elevation`, `mouseSens` (public), and private `locked_`/`firstMouse_`/`lastX_`/`lastY_`.
  - **Event-driven** — driven by a GLFW cursor pos callback, NOT polling. Accumulates every raw delta during `glfwPollEvents()`.
  - `onCursorPos(double x, double y)` — called by the cursor pos callback; ignores events when unlocked; guards the first event after lock to avoid a jump.
  - `setLocked(bool)` — syncs the locked state; automatically resets the first-mouse guard when locking.
  - `lookDir()` — returns the `glm::vec3` view direction from azimuth + elevation.
  - `WinCtx` struct in `main.cpp` holds `AppState*` + `Camera*` and is stored via `glfwSetWindowUserPointer`, allowing both the framebuffer and cursor pos callbacks to reach game state.
  - Cursor pos callback is registered **before** `ImGui_ImplGlfw_InitForOpenGL` so ImGui auto-chains it — our callback fires first, then ImGui's.

  ### Player
  - Collision: horizontal AABB test at `oldY + 0.1` to avoid vertical motion false-triggers.
  - Landing: checks feet Y against `platformTopY` within XZ footprint.
  - Respawn: falls below `FALL_LIMIT = -20.0f` → teleport to `spawnPos_`.
  - `eyePosition()` = `position + (0, 1.6, 0)`.

  ### Lighting
  - Point light at `player.eyePosition()` each frame via `u_LightPos` uniform.
  - Fragment normal uses interpolated `vNormal` (computed in vertex shader as `mat3(transpose(inverse(u_Model))) * aNormal`, correctly handles model rotation).
    - Previously used `dFdx`/`dFdy` on `vFragPos`, but this caused black shimmer at triangle edges: the GPU's 2×2 pixel quads straddle triangle boundaries, producing garbage derivatives and a random normal that faces away from the light.
  - Normal orientation: `geomN = normalize(vNormal) * (gl_FrontFacing ? 1.0 : -1.0)` — corrects for FBX winding without backface culling (assets use CW winding, so `glEnable(GL_CULL_FACE)` kills all geometry). Old upward-flip hack removed.

  ### Texture mapping modes (u_UseAffine)
  - Toggled via **"Retro Texture Warp"** checkbox in the ImGui stats panel (Tab to open).
  - `u_UseAffine = false` (default): **object-space triplanar** — projects from all 3 model-space axes using `vModelPos`; blend weights derived from model-space geometric normal (`normalize(cross(dFdx(vModelPos), dFdy(vModelPos)))`). Both UVs and blend weights are in model space, so the texture is locked to the mesh and projects correctly on any face orientation regardless of world rotation.
  - `u_UseAffine = true`: **world-space XZ affine** — `vUV_Affine` is set to `vec2(vFragPos.x, vFragPos.z)` in the vertex shader (world-space XZ, not mesh UVs), interpolated without perspective correction (`noperspective`). The fragment shader samples with `fract(vUV_Affine * uTileSize)`. This gives a flat top-down projection with affine rasterization warp artifacts (not classic PS1 mesh-UV warping).
  - Both branches are resolved in the fragment shader only; `vUV_Affine` is always emitted by the vertex shader.

  ### Normal mapping (u_UseNormalMap)
  - Toggled via **"Normal Mapping"** checkbox in the ImGui stats panel. On by default.
  - Only active in triplanar mode (no normal mapping in affine/retro mode).
  - **Triplanar normal mapping** — no per-vertex tangents needed. Each projection axis defines its own analytical TBN:
    - X-proj (UV=`.yz`): T=+Y, B=+Z, N=±X → model-space = `(b·sign, r, g)`
    - Y-proj (UV=`.xz`): T=+X, B=+Z, N=±Y → model-space = `(r, b·sign, g)`
    - Z-proj (UV=`.xy`): T=+X, B=+Y, N=±Z → model-space = `(r, g, b·sign)`
  - `sign(mN.x/y/z)` from `cross(dFdx(vModelPos), dFdy(vModelPos))` determines which face side we're on; `mN` is used for both blend weights (`abs`) and signs.
  - After transforming blended model-space normal to world space via normal matrix, hemisphere-check against `geomN` corrects any sign errors from `mN`.
  - **Known issue / IN PROGRESS**: normal mapping on the ground still needs verification after `gl_FrontFacing` fix. The sign determination via `mN` may still be inconsistent on some geometry. If problems persist, consider passing model-space vertex normal as a separate varying (`vModelNormal = aNormal` in vert shader) to replace `mN` for sign determination.

  ### Asset loading
  - All assets resolved relative to the executable via `exePath()`.
    - Linux: reads `/proc/self/exe`
    - Windows: uses `GetModuleFileNameW` (`WIN32_LEAN_AND_MEAN` + `<windows.h>`, guarded by `#ifdef _WIN32`)
  - `configure_file(... COPYONLY)` in CMakeLists.txt copies assets to the build dir at configure time.
  - Assimp builds as a **static lib** (`libassimp.a`) when cross-compiled with MinGW — baked into the exe, no DLL to ship.
  - POST_BUILD `copy_if_different` still present for shared-lib Linux builds where `libassimp.so` is needed.

  ### Windows distribution
  - Cross-compile from Linux: `cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw.cmake && cmake --build build-win -j$(nproc)`
  - MinGW runtime statically linked (`-static-libgcc -static-libstdc++`) — no `libstdc++` / `libgcc` DLLs needed.
  - Release zip (`VLoom-windows.zip`) contains only runtime files: `cube_renderer.exe`, `bricks_color.jpg`, `bricks_normal.jpg`, `*.fbx`, `shaders/`, `THIRD_PARTY_LICENSES.txt`
  - Recreate the zip: `cd build-win && zip -r ../VLoom-windows.zip cube_renderer.exe bricks_color.jpg bricks_normal.jpg ground.fbx ramp1.fbx railing.fbx shaders/ ../THIRD_PARTY_LICENSES.txt`

  ### Controls
  | Key | Action |
  |-----|--------|
  | WASD | Move |
  | Space | Jump |
  | Mouse | Look (FPS) |
  | Tab | Toggle cursor lock / show ImGui stats panel |
  | F1 | Wireframe debug mode (also prints pos/collision/grounded 1×/sec) |
  | Escape | Quit |

  ## Known/pending refactors
  - **`Shader` dead methods**: `use()`, `setInt/Float/Vec3/Mat4` on `Shader` are unused — all uniform/bind calls go through `RenderDevice` now. Safe to strip.
  - **`Texture::bind()` dead method**: `RenderDevice::bindTexture()` calls GL directly and does not delegate to `tex.bind()`, so `Texture::bind()` is unreachable in normal usage. Safe to remove.
  - **`RenderDevice::loc()` is uncached**: calls `glGetUniformLocation` on every uniform set, every frame. A per-shader `unordered_map<string, GLint>` cache would eliminate repeated string lookups.
  - ~~**No `TextureCache`**~~: implemented — `TextureCache.hpp`, `SceneObject::tex` is now a non-owning pointer.
  - ~~**`Entity` vs `SceneObject` asymmetry**~~: resolved — platform converted to `SceneObject` with TRS (pos=(0,-1,-5), rot=(90,0,0)°, scale=(20,20,0.2)). `Entity.hpp` is now dead code.

  ## Planned features (not yet started)
  - ~~**Save/load scene**~~: implemented — `SceneSerializer.hpp`, INI-style `scene.txt` next to the exe. Save/Load buttons in the Tab panel. Stores label, meshPath, texPath, pos, rot, scale, tile per object. Ground platform is NOT saved (it's hardcoded collision floor).
  - **Instanced rendering**: group SceneObjects by mesh path → `glDrawElementsInstanced`. Requires instance VBO with per-instance `mat4` (attribs 3–6, divisor=1) and a grouped render pass.

  ## Asset library (assets/Pieces/)
  Binary FBX files (`Kaydara FBX Binary`). 79 pieces total:
  - **Ground**: ground, ground1, ground corner
  - **Ramps**: ramp, ramp1
  - **Stairs**: stairs, stairs1, stairs2, stairs corner, stairs corner1
  - **Walls**: wall, wall1, wall corner, wall corner1, wall corner4, wall corner bottom, wall corner bottom1, wall corner bottom2, wall corner top, wall corner top1, wall door, wall door1, wall door2, wall window, wall window1
  - **Doors/Windows**: door, door1, door2, door3, window, window1
  - **Railings/Fences**: railing, railing edge, fence, fence2, fence3, fence edge, fence wood
  - **Pillars**: pillar, pillar1, pillar2, pillar3, pillar4
  - **Stairs**: (see above)
  - **Primitives**: cube, cube4–9, cylinder, cylinder1–5, sphere, sphere1, sphere2, cone, cone1–4, torus, box
  - **Props**: ladder, ladder1, spike, spikes big, spikes small, coin, key, arrow, toggle switch

  ## Texture library (assets/Textures/)
  - **Bricks076A_1K-JPG (2)/**: Full PBR set — Color (active), AO, Displacement, NormalGL, NormalDX, Roughness. Only Color is copied to the build dir currently. NormalGL is ready for normal mapping when the shader supports it.
