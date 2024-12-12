#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "config.h"
#include <math.h>
#include <iostream>
#include <vector>

enum Tool {
  BRUSH,
  BOX
};

class Game {
  public:
    void setup();
    void update();
    void render();

    // functions purely for organisation
    void renderUI();
    void processKeyboardInput();
    void processMouseInput();

  private:
    Vector2 boxPosition;
    float boxSize;
    Tool tool;
    Shader shader;
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
