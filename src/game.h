#ifndef GAME_H
#define GAME_H

#include <math.h>
#include <iostream>
#include <sstream>
#include <cctype>
#include <vector>
#include "config.h"
#include "raylib.h"
#include "raymath.h"
#include "imgui.h"
#include "rlImGui.h"

enum LightType {
  STATIC = 0,
  SINE,
  FLICKERING
};

struct Light {
  Vector2   position;
  Vector3   color;
  float     radius; // in pixels
  float     timeCreated;
  LightType type;
};

struct WindowData {
  ImGuiWindowFlags flags = 0;
  bool open              = true;
};

struct ImageTexture {
  Image     img;
  Texture2D tex;
};

class Game {
  public:
    Game();
    void update();
    void render();
    void renderUI();
    void processKeyboardInput();
    void processMouseInput();

    void addLight(Vector2 positon, Vector3 normalisedColor, float radius, LightType type);
    void placeLights(int lightNumber = 4, float distFromCentre = 256.0);
    void reloadCanvas();
    void clearCanvas();

  private:
    Shader lightingShader;
    int cascadeAmount;
    float time;
    WindowData debugWindowData;
    std::vector<Light> lights;

    bool debug;
    bool randomLightColor;
    bool randomLightSize;
    bool randomLightType;
    bool perspective;
    bool skipUIRendering;

    int currentMap = 0;
    const std::string maps[2] = { "maze.png", "trees.png" };

    ImageTexture canvas;
    ImageTexture cursor;

    enum {
      DRAWING,
      LIGHTING,
      VIEWING
    } mode;

    struct {
      Image     img;
      Texture2D tex;
      float     brushSize;
      float     lightSize;
      Color     lightColor;
      int       lightType;
    } brush;

    struct {
      float size;
      Color color;
    } light;
};

#endif /* GAME_H */
