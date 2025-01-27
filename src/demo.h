#ifndef DEMO_H
#define DEMO_H

#include <iostream>
#include <vector>
#include "config.h"
#include "raylib.h"
#include "raymath.h"
#include "imgui.h"
#include "rlImGui.h"

enum LightType {
  STATIC = 0,
  SINE,
  SAW,
  NOISE
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

enum Mode {
  DRAWING,
  LIGHTING,
  VIEWING
};

class Demo {
  public:
    Demo();
    void update();
    void render();
    void renderUI();
    void processKeyboardInput();
    void processMouseInput();
    void clear();

  private:
    void addLight(Vector2 position, Vector3 normalisedColor, float radius, LightType type);
    void placeLights(int lightNumber = 4, float distFromCentre = 256.0);
    void reload();

    // user
    struct {
      Mode         mode;
      ImageTexture brush;
      float        brushSize;
      float        lightSize;
      Color        lightColor;
      int          lightType;
    } user;

    bool randomLightColor;
    bool randomLightSize;
    bool randomLightType;
    double timeSinceLastType;

    bool perspective;

    // ui
    bool debug;
    bool help;
    WindowData debugWindowData;
    bool skipUIRendering;

    std::vector<Light> lights;

    const std::string maps[2] = { "maze.png", "trees.png" };

    // resources
    int currentMap = 0;
    Shader lightingShader;
    ImageTexture canvas;
    ImageTexture cursor;

    // misc
    int cascadeAmount;
};

#endif /* DEMO_H */
