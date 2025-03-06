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

    struct {
      Mode         mode;
      ImageTexture brush;
      float        brushSize;
      Color        lightColor;
    } user;

    int maxSteps;
    int jfaSteps;
    int raysPerPx;
    bool debug;
    bool sceneHasChanged;

    // UI

    bool skipUIRendering;
    bool debugShowBuffers;
    WindowData debugWindowData;
    bool help;
    Vector2 lastMousePos;

    // RESOURCES
    std::map<std::string, Shader> shaders;

    ImageTexture occlusionMap;
    ImageTexture emissionMap;

    Texture cursorTex;

    RenderTexture2D sceneBuf;
    RenderTexture2D bufferA;
    RenderTexture2D bufferB;
    RenderTexture2D bufferC;
    RenderTexture2D distFieldBuf;
};

#endif /* DEMO_H */
