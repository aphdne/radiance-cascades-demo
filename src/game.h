#ifndef GAME_H
#define GAME_H

#include <math.h>
#include <iostream>
#include <vector>
#include "config.h"
#include "raylib.h"
#include "raymath.h"
#include "imgui.h"
#include "rlImGui.h"

struct Light {
  Vector2 position;
  Vector3 color;
  float   radius; // in pixels
};

struct DebugWindowData {
  ImGuiWindowFlags flags = 0;
  bool open              = true;
};

class Game {
  public:
    Game();
    void update();
    void render();
    void renderUI();
    void processKeyboardInput();
    void processMouseInput();

  private:
    Shader lightingShader;
    int cascadeAmount;

    bool debug;
    float time;
    double timeSinceModeSwitch;

    bool skipUIRendering;

    Texture2D currentToolIcon;
    DebugWindowData debugWindowData;

    std::vector<Light> lights;

    bool viewing = false;

    enum Mode {
      DRAWING,
      LIGHTING,
      VIEWING
    } mode;

    struct {
      Image     img;
      Texture2D tex;
    } cursor;

    struct {
      Image     img;
      Texture2D tex;
      float     scale;
      Color     color;
    } brush;

    struct {
      Image img;
      Texture2D tex;
    } canvas;
};

#endif /* GAME_H */
