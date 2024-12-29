#include "game.h"

int main() {
  if (!DirectoryExists("res")) {
    printf("Please run this file from the project root directory.\n");
    return 0;
  }

  std::string title = "Radiance Cascades ";
  title += VERSION_STAGE;
  title += VERSION;

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, title.c_str());
  SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
  SetTraceLogLevel(LOG_WARNING);
  rlImGuiSetup(true);

  Game game;

  while (!WindowShouldClose())
  {
    game.processKeyboardInput();
    game.processMouseInput();
    game.update();
    BeginDrawing();
      game.render();
      rlImGuiBegin();
        game.renderUI();
      rlImGuiEnd();
    EndDrawing();
  }

  rlImGuiShutdown();
  CloseWindow();
}
