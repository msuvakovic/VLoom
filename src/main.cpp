#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <filesystem>
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

#include "Shader.hpp"
#include "Mesh.hpp"
#include "Player.hpp"
#include "Texture.hpp"
#include "SceneObject.hpp"
#include "SceneEditor.hpp"
#include "SceneSerializer.hpp"
#include "RenderDevice.hpp"
#include "MeshCache.hpp"
#include "TextureCache.hpp"
#include "Camera.hpp"

static const int WIDTH  = 800;
static const int HEIGHT = 600;

static std::string exePath(const char* rel) {
    namespace fs = std::filesystem;
#ifdef _WIN32
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return (fs::path(buf).parent_path() / rel).string();
#else
    return (fs::read_symlink("/proc/self/exe").parent_path() / rel).string();
#endif
}


struct AppState {
    float aspectRatio       = (float)WIDTH / HEIGHT;
    bool  debugMode         = false;
    bool  cursorLocked      = true;
    bool  useAffineMapping  = false;
    bool  useNormalMap      = true;
    bool  prevF1            = false;  // edge-detection for polled toggle keys
    bool  prevTab           = false;
};

// Passed via glfwSetWindowUserPointer so both callbacks can reach game state.
struct WinCtx { AppState* state; Camera* cam; };

int main() {
    if (!glfwInit()) { std::cerr << "GLFW init failed\n"; return 1; }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* win = glfwCreateWindow(WIDTH, HEIGHT, "VLoom", nullptr, nullptr);
    if (!win) { std::cerr << "Window creation failed\n"; glfwTerminate(); return 1; }

    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD init failed\n"; return 1;
    }

    RenderDevice rd;
    rd.setDepthTest(true);
    rd.setViewport(0, 0, WIDTH, HEIGHT);

    AppState state;
    Camera   camera;
    WinCtx   winCtx{&state, &camera};
    glfwSetWindowUserPointer(win, &winCtx);

    glfwSetFramebufferSizeCallback(win, [](GLFWwindow* w, int fw, int fh) {
        glViewport(0, 0, fw, fh);
        auto* ctx = static_cast<WinCtx*>(glfwGetWindowUserPointer(w));
        ctx->state->aspectRatio = (fh > 0) ? (float)fw / fh : 1.0f;
    });

    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(win, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    // Cursor pos callback — set BEFORE ImGui init so ImGui auto-chains it.
    // Accumulates every raw delta into Camera during glfwPollEvents().
    glfwSetCursorPosCallback(win, [](GLFWwindow* w, double x, double y) {
        auto* ctx = static_cast<WinCtx*>(glfwGetWindowUserPointer(w));
        ctx->cam->onCursorPos(x, y);
    });

    // ImGui init — install_callbacks=true lets ImGui handle its own text-input needs
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Save the default style so we can scale from a clean baseline each time
    ImGuiStyle defaultStyle = ImGui::GetStyle();
    static const float    uiScales[]  = { 1.0f, 1.5f, 2.0f };
    static const char*    uiScaleNames[] = { "Small", "Medium", "Large" };
    int uiScaleIdx = 0;

    // ── Scene setup ──────────────────────────────────────────────────────────
    Shader shader = Shader::fromFiles(
        exePath("shaders/default.vert").c_str(),
        exePath("shaders/default.frag").c_str()
    );

    MeshCache    meshCache;
    TextureCache texCache;

    SceneObject platform;
    platform.label = "Ground";
    platform.mesh  = &meshCache.get(exePath(ASSET_PATH));
    if (!platform.mesh->vao) { glfwTerminate(); return 1; }
    platform.tex     = &texCache.get(exePath("bricks_color.jpg"));
    platform.pos     = glm::vec3(0.0f, -1.0f, -5.0f);
    platform.rot     = glm::vec3(90.0f, 0.0f, 0.0f);
    platform.scale   = glm::vec3(20.0f, 20.0f, 0.2f);
    platform.texTile = 4.0f;
    platform.rebuildTransform();

    std::vector<SceneObject> objects;

    {
        SceneObject obj;
        obj.label    = "Ramp";
        obj.meshPath = "ramp1.fbx";
        obj.texPath  = "bricks_color.jpg";
        obj.mesh     = &meshCache.get(exePath("ramp1.fbx"));
        obj.tex      = &texCache.get(exePath("bricks_color.jpg"));
        obj.pos      = glm::vec3(10.0f, 0.0f, 10.0f);
        obj.rebuildTransform();
        objects.push_back(std::move(obj));
    }
    {
        SceneObject obj;
        obj.label    = "Railing";
        obj.meshPath = "railing.fbx";
        obj.texPath  = "bricks_color.jpg";
        obj.mesh     = &meshCache.get(exePath("railing.fbx"));
        obj.tex      = &texCache.get(exePath("bricks_color.jpg"));
        obj.rebuildTransform();
        objects.push_back(std::move(obj));
    }

    const Texture& normalTex = texCache.get(exePath("bricks_normal.jpg"));

    rd.bindShader(shader);
    rd.setInt("uTex", 0);
    rd.setInt("uNormalMap", 1);

    float platformTopY = platform.worldAABB().second.y;
    Player player(glm::vec3(0.0f, platformTopY + 0.1f, -5.0f));

    SceneEditor editor;

    // ── Main loop ────────────────────────────────────────────────────────────
    double prevTime      = glfwGetTime();
    float  debugPrintAcc = 0.0f;

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();

        double now = glfwGetTime();
        float  dt  = float(now - prevTime);
        prevTime   = now;

        // ImGui new frame first — this updates WantCaptureMouse / WantCaptureKeyboard
        // so the flags are fresh before any game input is processed this frame.
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        const ImGuiIO& io = ImGui::GetIO();

        // ── Toggle keys — polled edge detection (no callback needed) ─────────
        if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(win, true);

        bool f1Now  = glfwGetKey(win, GLFW_KEY_F1)  == GLFW_PRESS;
        bool tabNow = glfwGetKey(win, GLFW_KEY_TAB) == GLFW_PRESS;
        if (f1Now && !state.prevF1) {
            state.debugMode = !state.debugMode;
            std::cout << "Debug mode: " << (state.debugMode ? "ON" : "OFF") << '\n';
        }
        if (tabNow && !state.prevTab) {
            state.cursorLocked = !state.cursorLocked;
            glfwSetInputMode(win, GLFW_CURSOR,
                             state.cursorLocked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            if (state.cursorLocked && glfwRawMouseMotionSupported())
                glfwSetInputMode(win, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
            camera.setLocked(state.cursorLocked);
        }
        state.prevF1  = f1Now;
        state.prevTab = tabNow;

        // ── Player update — skip movement input when ImGui owns the keyboard ─
        // Camera is updated by the cursor pos callback during glfwPollEvents() above.
        const bool allowMove = state.cursorLocked || !io.WantCaptureKeyboard;
        player.update(win, dt, camera.azimuth, platform, allowMove);

        glm::vec3 eye     = player.eyePosition();
        glm::vec3 lookDir = camera.lookDir();

        glm::mat4 proj = glm::perspective(glm::radians(45.0f), state.aspectRatio, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(eye, eye + lookDir, glm::vec3(0.0f, 1.0f, 0.0f));

        if (state.debugMode) {
            debugPrintAcc += dt;
            if (debugPrintAcc >= 1.0f) {
                debugPrintAcc = 0.0f;
                std::cout << "[DEBUG] pos=(" << player.position.x << ", "
                          << player.position.y << ", " << player.position.z
                          << ")  colliding=" << (player.lastColliding ? "YES" : "no")
                          << "  grounded="  << (player.isGrounded    ? "yes" : "no") << '\n';
            }
        } else {
            debugPrintAcc = 0.0f;
        }

        if (!state.cursorLocked) {
            ImGui::Begin("Stats", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text("FPS: %.1f  (%.2f ms)", ImGui::GetIO().Framerate,
                                                 1000.0f / ImGui::GetIO().Framerate);

            ImGui::Separator();
            ImGui::Text("UI Scale:");
            ImGui::SameLine();
            if (ImGui::Button(uiScaleNames[uiScaleIdx])) {
                uiScaleIdx = (uiScaleIdx + 1) % 3;
                ImGui::GetStyle() = defaultStyle;
                ImGui::GetStyle().ScaleAllSizes(uiScales[uiScaleIdx]);
                ImGui::GetIO().FontGlobalScale = uiScales[uiScaleIdx];
            }

            ImGui::Separator();
            ImGui::Checkbox("Retro Texture Warp", &state.useAffineMapping);
            ImGui::Checkbox("Normal Mapping",     &state.useNormalMap);

            ImGui::Separator();
            if (ImGui::Button("Save Scene")) {
                SceneSerializer::save(exePath("scene.txt"), objects);
            }
            ImGui::SameLine();
            if (ImGui::Button("Load Scene")) {
                SceneSerializer::load(exePath("scene.txt"), meshCache, texCache, objects);
            }

            ImGui::Separator();
            editor.draw(objects);

            ImGui::End();
        }

        rd.clear(0.05f, 0.05f, 0.07f);

        if (state.debugMode) rd.setPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        rd.bindShader(shader);
        rd.setMat4("u_Projection", proj);
        rd.setMat4("u_View", view);
        rd.setVec3("u_LightPos", eye);
        rd.setFloat("u_LightStrength", 1.0f);
        rd.setInt("u_UseAffine",    state.useAffineMapping ? 1 : 0);
        rd.setInt("u_UseNormalMap", state.useNormalMap    ? 1 : 0);
        if (normalTex.isValid()) rd.bindTexture(normalTex, GL_TEXTURE1);
        platform.draw(rd);
        for (auto& obj : objects) obj.draw(rd);

        if (state.debugMode) rd.setPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(win);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}
