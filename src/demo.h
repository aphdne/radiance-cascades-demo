#ifndef DEMO_H
#define DEMO_H

#include <string>
#include <map>
#include <iostream>
#include "config.h"
#include "raylib.h"
#include "raymath.h"
#include "imgui.h"
#include "rlImGui.h"
#include "rlgl.h"

struct WindowData {
  ImGuiWindowFlags flags = 0;
  bool open              = true;
};

enum Mode {
  DRAWING,
  LIGHTING,
};

class Demo {
  public:
    Demo();
    // void update();
    void render();
    void renderUI();
    void processKeyboardInput();
    void processMouseInput();
    void resize();

  private:
    void userSetRandomColor();
    void loadShader(std::string shader);
    void setScene(int scene);
    void setBuffers();
    void saveCanvas();

    struct {
      Mode         mode;
      Texture      brushTexture;
      float        brushSize;
      Color        brushColor;
    } user;

    bool debug;
    bool gi;

    // shader settings
    // global
    bool orbs;
    bool mouseLight;
    int maxRaySteps;
    int jfaSteps;
    bool srgb;
    bool drawRainbow;
    bool rainbowAnimation;
    float mixFactor;
    float propagationRate;

    // gi
    int giRayCount;
    bool giNoise;

    // rc
    int rcRayCount;
    int cascadeAmount;
    int cascadeDisplayIndex;
    bool rcBilinear;
    bool rcDisableMerging;
    float baseInterval;
    bool ambient;
    Vector3 ambientColor;

    // UI

    bool skipUIRendering;
    bool debugShowBuffers;
    Vector2 lastMousePos;
    unsigned short framesSinceLastMousePos;
    WindowData infoWindowData;
    WindowData colorWindowData;
    WindowData settingsWindowData;
    WindowData screenshotWindowData;
    float timeSinceScreenshot;
    int displayNumber;
    int selectedScene;

    const Texture2D UI_0  = LoadTexture("res/textures/ui/0_trad16rays.png");
    const Texture2D UI_1  = LoadTexture("res/textures/ui/1_trad4rays.png");
    const Texture2D UI_2  = LoadTexture("res/textures/ui/2_trad4raystoofar.png");
    const Texture2D UI_3  = LoadTexture("res/textures/ui/3_trad16rays.png");
    const Texture2D UI_4  = LoadTexture("res/textures/ui/4_radianceinterval.png");
    const Texture2D UI_5A = LoadTexture("res/textures/ui/5a_penumbra.png");
    const Texture2D UI_5  = LoadTexture("res/textures/ui/5_bilinear.png");
    const Texture2D UI_6  = LoadTexture("res/textures/ui/6_cascade0.png");
    const Texture2D UI_7  = LoadTexture("res/textures/ui/7_cascade1.png");
    const Texture2D UI_8  = LoadTexture("res/textures/ui/8_cascade2.png");

    // RESOURCES
    std::map<std::string, Shader> shaders;

    const Texture2D cursorTex = LoadTexture("res/textures/cursor.png");

    RenderTexture2D* displayBuffer;
    RenderTexture2D occlusionBuf;
    RenderTexture2D emissionBuf;
    RenderTexture2D sceneBuf;
    RenderTexture2D jfaBufferA;
    RenderTexture2D jfaBufferB;
    RenderTexture2D jfaBufferC;
    RenderTexture2D distFieldBuf;
    RenderTexture2D radianceBufferA;
    RenderTexture2D radianceBufferB;
    RenderTexture2D radianceBufferC;
    RenderTexture2D lastFrameBuf;
};

#endif /* DEMO_H */
