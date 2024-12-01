#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "config.h"
#include <math.h>
#include <iostream>

class Game {
  public:
    void setup();
    void update();
    void render();

  private:
    Vector2 boxPosition;
    float boxSize;
    Shader shader;
    float time;
};

#endif /* GAME_H */
