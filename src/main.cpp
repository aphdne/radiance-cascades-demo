#include "raylib.h"
#include "game.h"

int main() {
  std::string title = "Radiance Cascades ";
  title += VERSION_STAGE;
  title += VERSION;

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, title.c_str());

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
