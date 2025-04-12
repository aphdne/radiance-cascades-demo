#ifndef DEMO_H
#define DEMO_H

#include <map>
#include <iostream>
// #include <vector>
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

struct ImageTexture {
  Image     img;
  Texture2D tex;
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

  private:
    void userSetRandomColor();
    void loadShader(std::string shader);
    void reload();
    void clear();
    void setScene(Scene scene);

    struct {
      Mode         mode;
      ImageTexture brush;
      float        brushSize;
      Color        lightColor;
    } user;

    bool debug;
    bool gi;
    // shader settings
    // global
    bool orbs;
    int maxRaySteps;
    int jfaSteps;
    bool srgb;

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

    // UI

    bool skipUIRendering;
    bool debugShowBuffers;
    WindowData debugWindowData;
    Vector2 lastMousePos;
    bool help;

    // RESOURCES
    std::map<std::string, Shader> shaders;

    ImageTexture occlusionMap;
    ImageTexture emissionMap;

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
};

#endif /* DEMO_H */
