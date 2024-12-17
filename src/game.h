#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "config.h"
#include <math.h>
#include <iostream>
#include <vector>

class Game {
  public:
    void setup();
    void update();
    void render();
    void renderUI();
    void processKeyboardInput();
    void processMouseInput();

  private:
    Vector2 boxPosition;
    float boxSize;
    enum {
      BRUSH,
      BOX
    } tool;
    Shader rainbowShader;
    Shader maskShader;
    bool debug;
    float time;
    Image brush;
    Texture2D brushTex;
    Image canvas;
    Texture2D canvasTex;

    struct {
      Rectangle rect;
      ushort x;
      ushort y;
    } boxToolInfo;

    std::vector<Rectangle*> walls;
};

#endif /* GAME_H */
