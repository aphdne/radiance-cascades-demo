#include "raylib.h"
#include "game.h"

int main() {
  std::string title = "Radiance Cascades ";
  title += VERSION_STAGE;
  title += VERSION;

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, title.c_str());
  SetTargetFPS(144);

  Game game;

  game.setup();
  while (!WindowShouldClose())
  {
    BeginDrawing();
    game.update();
      game.render();
    EndDrawing();
  }

  CloseWindow();
}
