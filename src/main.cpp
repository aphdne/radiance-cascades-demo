#include "raylib.h"
#include "game.h"

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600 

#define GLSL_VERSION 330

int main() {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Radiance Cascades");
  
  Game game;

  game.setup();
  while (!WindowShouldClose())
  {
    game.update();
    BeginDrawing();
      game.render();
    EndDrawing();
  }

  CloseWindow();
}
