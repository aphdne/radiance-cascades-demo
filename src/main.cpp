#include "demo/drawing_demo.h"
#include "demo/pong_demo.h"

int main() {
  if (!DirectoryExists("res")) {
    printf("Please run this file from the project root directory.\n");
    return 0;
  }

  std::string title = "Radiance Cascades DEGREE SHOW EDITION ";
  // title += VERSION_STAGE;
  // title += VERSION;

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(1600, 900, title.c_str());
  SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
  SetTraceLogLevel(LOG_WARNING);
  rlImGuiSetup(true);

  DrawingDemo drawing;
  PongDemo pong;
  Demo* demo = &drawing;

  int screenWidth = GetScreenWidth();
  int screenHeight = GetScreenHeight();
  while (!WindowShouldClose())
  {
    if (IsKeyPressed(KEY_P)) {
      (demo == &pong) ? demo = &drawing : demo = &pong;
    }

    demo->processMouseInput();
    demo->processKeyboardInput();
    demo->update();
    BeginDrawing();
      demo->render();
      rlImGuiBegin();
        demo->renderUI();
      rlImGuiEnd();
      if (screenWidth != GetScreenWidth() || screenHeight != GetScreenHeight()) {
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();
        demo->resize();
      }
    EndDrawing();
  }

  rlImGuiShutdown();
  CloseWindow();
}
