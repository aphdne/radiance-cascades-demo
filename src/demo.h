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
    void reloadShaders();
    void reload();
    void clear();

    // user
    struct {
      Mode         mode;
      ImageTexture brush;
      float        brushSize;
      Color        lightColor;
    } user;

    bool perspective;
    int maxSteps;
    int raysPerPx;

    // ui
    bool debug;
    bool help;
    WindowData debugWindowData;
    bool skipUIRendering;

    const std::string maps[2] = { "maze.png", "trees.png" }; // needed in this datatype for imgui

    // resources
    int currentMap = 0;
    std::map<std::string, Shader> shaders;
    ImageTexture occlusionMap;
    ImageTexture emissionMap;
    ImageTexture cursor;

    // misc
    int cascadeAmount;
};

#endif /* DEMO_H */
