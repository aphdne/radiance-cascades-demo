#ifndef DEMO_H
#define DEMO_H

#include <map>
#include <string>
#include <iostream>
#include "config.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "imgui.h"
#include "rlImGui.h"

#define SCREENSHOT_POPUP_TIME 2 // seconds
#define WINDOW_OPACITY 0.35

struct WindowData {
  ImGuiWindowFlags flags{0};
  bool open{true};
};

class Demo {
  public:
    Demo();
    virtual void processMouseInput();
    virtual void processKeyboardInput();
    virtual void resize();
    virtual void update() {}
    void render();
    virtual void renderUI();
  protected:
    virtual void prepScene() {}
    static void saveCanvas();

    static inline WindowData screenshotWindowData{0, true};
    static inline bool skipUIRendering{false};

    static inline float timeSinceScreenshot{0};
    static inline bool debug{false};
    static inline bool orbs{false};
    static inline bool mouseLight{true};
    static inline int maxRaySteps{256};
    static inline int jfaSteps{512};
    static inline bool srgb{true};
    static inline bool drawRainbow{false};
    static inline bool rainbowAnimation{false};
    static inline float mixFactor{0.7};
    static inline float propagationRate{1.3};
    static inline bool ambient{false};
    static inline Vector3 ambientColor{Vector3(1.0, 1.0, 1.0)};

    static inline bool gi{false};
    static inline int giRayCount{64};
    static inline bool giNoise{true};

    static inline int rcRayCount{4};
    static inline int cascadeAmount{5};
    static inline int cascadeDisplayIndex{0};
    static inline bool rcBilinear{true};
    static inline bool rcDisableMerging{false};
    static inline float baseInterval{0.5};

    static inline std::map<std::string, Shader> shaders{};

    // initialise buffers with junk; this is resolved in setBuffers()
    // initialising with LoadRenderTexture() compiles but causes a segfault at runtime
    static inline RenderTexture2D occlusionBuf{};
    static inline RenderTexture2D emissionBuf{};
    static inline RenderTexture2D sceneBuf{};

  private:
    static void loadShader(std::string shader);
    static void setBuffers();
    static void renderScene();

    static inline RenderTexture2D jfaBufferA{};
    static inline RenderTexture2D jfaBufferB{};
    static inline RenderTexture2D jfaBufferC{};
    static inline RenderTexture2D distFieldBuf{};
    static inline RenderTexture2D radianceBufferA{};
    static inline RenderTexture2D radianceBufferB{};
    static inline RenderTexture2D radianceBufferC{};
    static inline RenderTexture2D lastFrameBuf{};
    static inline RenderTexture2D* displayBuffer = &lastFrameBuf;
};

#endif /* DEMO_H */
