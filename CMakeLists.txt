cmake_minimum_required(VERSION 3.16)
project(cube_renderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ── GLFW ──────────────────────────────────────────────────────────────────────
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND  OFF CACHE BOOL "" FORCE)
if(WIN32)
    set(GLFW_BUILD_X11   OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WIN32 ON  CACHE BOOL "" FORCE)
else()
    set(GLFW_BUILD_X11   ON  CACHE BOOL "" FORCE)
endif()
FetchContent_MakeAvailable(glfw)

# ── GLM ───────────────────────────────────────────────────────────────────────
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glm)

# ── GLAD ──────────────────────────────────────────────────────────────────────
FetchContent_Declare(glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG        v0.1.36
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(glad)
if(NOT glad_POPULATED)
    FetchContent_Populate(glad)
    set(GLAD_PROFILE   "core"  CACHE STRING "" FORCE)
    set(GLAD_API       "gl=3.3" CACHE STRING "" FORCE)
    set(GLAD_GENERATOR "c"     CACHE STRING "" FORCE)
    add_subdirectory(${glad_SOURCE_DIR} ${glad_BINARY_DIR})
endif()

# ── Assimp ────────────────────────────────────────────────────────────────────
FetchContent_Declare(assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG        v5.3.1
    GIT_SHALLOW    TRUE
)
set(ASSIMP_BUILD_TESTS              OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS       OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES            OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT                ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER       ON  CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(assimp)

# ── ImGui ─────────────────────────────────────────────────────────────────────
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.8
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

# ── stb ───────────────────────────────────────────────────────────────────────
FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
endif()

# ── Executable ────────────────────────────────────────────────────────────────
add_executable(cube_renderer src/main.cpp src/stb_image.cpp)
target_sources(cube_renderer PRIVATE
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(cube_renderer PRIVATE
    ${stb_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(cube_renderer PRIVATE glfw glad glm::glm assimp)
if(MINGW)
    # Statically link MinGW runtime so the .exe doesn't need libstdc++-6.dll / libgcc_s_uc0-1.dll
    target_link_options(cube_renderer PRIVATE -static-libgcc -static-libstdc++)
endif()
target_compile_definitions(cube_renderer PRIVATE
    ASSET_PATH="ground.fbx")

configure_file("${CMAKE_SOURCE_DIR}/assets/Textures/Bricks076A_1K-JPG (2)/Bricks076A_1K-JPG_Color.jpg"    ${CMAKE_BINARY_DIR}/bricks_color.jpg  COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/assets/Textures/Bricks076A_1K-JPG (2)/Bricks076A_1K-JPG_NormalGL.jpg" ${CMAKE_BINARY_DIR}/bricks_normal.jpg COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/assets/Pieces/ground.fbx  ${CMAKE_BINARY_DIR}/ground.fbx     COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/assets/Pieces/ramp1.fbx    ${CMAKE_BINARY_DIR}/ramp1.fbx    COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/assets/Pieces/railing.fbx  ${CMAKE_BINARY_DIR}/railing.fbx  COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/shaders/default.vert      ${CMAKE_BINARY_DIR}/shaders/default.vert COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/shaders/default.frag      ${CMAKE_BINARY_DIR}/shaders/default.frag COPYONLY)

# Copy libassimp next to the executable after each build
add_custom_command(TARGET cube_renderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:assimp>
        $<TARGET_FILE_DIR:cube_renderer>
)
