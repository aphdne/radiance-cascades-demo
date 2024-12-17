#include "raylib.h"
#include "config.h"
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
    game.processKeyboardInput();
    game.processMouseInput();
    game.update();
    BeginDrawing();
      game.render();
      game.renderUI();
    EndDrawing();
  }

  CloseWindow();
}
