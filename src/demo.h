#ifndef DEMO_H
#define DEMO_H

#include <map>
#include <iostream>
#include <vector>
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
    int raysPerPx;
    bool debug;

    // UI

    // for shader uniforms
    Vector2 resolution;

    bool skipUIRendering;
    WindowData debugWindowData;
    bool help;

    // RESOURCES
    std::map<std::string, Shader> shaders;

    ImageTexture occlusionMap;
    ImageTexture emissionMap;

    Texture cursorTex;
};

#endif /* DEMO_H */
