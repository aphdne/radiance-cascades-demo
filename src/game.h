#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include <math.h>
#include <iostream>

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600 

class Game {
  public:
    void setup();
    void update();
    void render();

  private:
    Vector2 boxPosition;
    float boxSize;
};

#endif /* GAME_H */
