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
  ImVec2 pos;
  ImVec2 size;
};

enum Mode {
  DRAWING,
  LIGHTING,
};

enum Scene {
  CLEAR,
  MAZE,
  TREES,
  PENUMBRA,
  PENUMBRA2
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
    void setScene(Scene scene);
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
    int maxRaySteps;
    int jfaSteps;
    bool srgb;
    bool drawRainbow;
    bool rainbowAnimation;

    // gi
    int giRayCount;
    bool giNoise;
    float giMixFactor;
    float giDecayRate;

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
    bool help;

    // RESOURCES
    std::map<std::string, Shader> shaders;

    Texture cursorTex;

    RenderTexture2D sceneBuf;
    RenderTexture2D tempBuf;
    RenderTexture2D bufferA;
    RenderTexture2D bufferB;
    RenderTexture2D bufferC;
    RenderTexture2D distFieldBuf;
    RenderTexture2D radianceBufferA;
    RenderTexture2D radianceBufferB;
    RenderTexture2D radianceBufferC;
    RenderTexture2D lastFrameBuf;
    RenderTexture2D occlusionBuf;
    RenderTexture2D emissionBuf;
};

#endif /* DEMO_H */
