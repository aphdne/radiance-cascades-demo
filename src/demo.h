#ifndef DEMO_H
#define DEMO_H

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
    void update();
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
    float decayRate;

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

    // UI

    bool skipUIRendering;
    bool debugShowBuffers;
    Vector2 lastMousePos;
    WindowData debugWindowData;
    WindowData sceneWindowData;
    WindowData colorWindowData;
    WindowData lightingWindowData;
    WindowData helpWindowData;
    bool help;
    int displayNumber;
    int selectedScene;

    // RESOURCES
    std::map<std::string, Shader> shaders;

    Texture cursorTex;

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
