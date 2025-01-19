#include "demo.h"

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

  Demo demo;

  while (!WindowShouldClose())
  {
    demo.processKeyboardInput();
    demo.processMouseInput();
    demo.update();
    BeginDrawing();
      demo.render();
      rlImGuiBegin();
        demo.renderUI();
      rlImGuiEnd();
    EndDrawing();
  }

  rlImGuiShutdown();
  CloseWindow();
}
