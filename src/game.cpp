#include "game.h"

#define MOUSE_VECTOR Vector2{ static_cast<float>(GetMouseX()), static_cast<float>(GetMouseY()) }
#define RELOAD_CANVAS() UnloadTexture(canvas.tex); \
                        canvas.tex = LoadTextureFromImage(canvas.img);

//                                           no. of lights, distance from centre
void placeLights(std::vector<Light>* lights, int N = 4, float d = 256.0) {
  for (float i = 0; i < N; i++) {
    float t = (i+1) * (PI*2/N) + 0.1;
    Light l;
    l.position = Vector2{ SCREEN_WIDTH/2 + std::sin(t) * d, SCREEN_HEIGHT/2 + std::cos(t) * d};
    l.color    = Vector3{ std::sin(t), std::cos(t), 1.0 };
    l.radius   = 300;
    lights->push_back(l);
  }
}

Game::Game() {
  debug = false;
  mode  = DRAWING;
  skipUIRendering = false;
  randomColor = false;

  lightingShader = LoadShader(0, "res/shaders/lighting.frag");
  if (!IsShaderValid(lightingShader)) {
    printf("lightingShader is broken!!\n");
    UnloadShader(lightingShader);
    lightingShader = LoadShader(0, "res/shaders/broken.frag");
  }

  cascadeAmount = 512;

  cursor.img = LoadImage("res/textures/cursor.png");
  cursor.tex = LoadTextureFromImage(cursor.img);

  brush.img = LoadImage("res/textures/brush.png");
  brush.tex = LoadTextureFromImage(brush.img);
  brush.scale = 0.25;

  std::string filepath = "res/textures/canvas/" + maps[currentMap];
  canvas.img = LoadImage(filepath.c_str());
  canvas.tex = LoadTextureFromImage(canvas.img);

  placeLights(&lights);

  debugWindowData.flags |= ImGuiWindowFlags_NoTitleBar;
  debugWindowData.flags |= ImGuiWindowFlags_NoScrollbar;
  debugWindowData.flags |= ImGuiWindowFlags_NoMove;
  debugWindowData.flags |= ImGuiWindowFlags_NoResize;
  debugWindowData.flags |= ImGuiWindowFlags_NoCollapse;
  debugWindowData.flags |= ImGuiWindowFlags_NoBackground;
  debugWindowData.flags |= ImGuiWindowFlags_NoNav;

  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = NULL;

  HideCursor();
}

void Game::update() {
  if      (brush.scale < 0.1) brush.scale = 0.1;
  else if (brush.scale > 1.0) brush.scale = 1.0;

  time = GetTime();
  SetShaderValue(lightingShader,    GetShaderLocation(lightingShader, "uTime"), &time, SHADER_UNIFORM_FLOAT);

  Vector2 resolution = { SCREEN_WIDTH, SCREEN_HEIGHT };
  resolution *= GetWindowScaleDPI();
  SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);

  Vector2 mouse = MOUSE_VECTOR * GetWindowScaleDPI();
  SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uPlayerLocation"), &mouse, SHADER_UNIFORM_VEC2);

  int lightsAmount = lights.size();
  SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uLightsAmount"), &lightsAmount, SHADER_UNIFORM_INT);
  SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uCascadeAmount"), &cascadeAmount, SHADER_UNIFORM_INT);

  int v = viewing;
  if (mode != VIEWING) v = 0;
  SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uViewing"), &v, SHADER_UNIFORM_INT);

  for (int i = 0; i < lights.size(); i++) {
    Vector2 position = lights[i].position * GetWindowScaleDPI();
    float   radius   = lights[i].radius * GetWindowScaleDPI().x;
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, TextFormat("lights[%i].position", i)), &position, SHADER_UNIFORM_VEC2);
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, TextFormat("lights[%i].color",    i)), &lights[i].color,    SHADER_UNIFORM_VEC3);
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, TextFormat("lights[%i].radius",   i)), &radius,   SHADER_UNIFORM_FLOAT);
   }

  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureMouse) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
  } else {
    ImGui::SetMouseCursor(ImGuiMouseCursor_None);
  }

  if (randomColor) brush.color = ColorFromNormalized(Vector4{ (std::sin(time) + 1) / 2, (std::cos(time) + 1) / 2, (std::sin(time*2) + 1) / 2, 1.0 });

}

void Game::render() {
  ClearBackground(PINK);

  BeginShaderMode(lightingShader);
    // <!> SetShaderValueTexture() has to be called while the shader is enabled
    SetShaderValueTexture(lightingShader, GetShaderLocation(lightingShader, "uOcclusionMask"), canvas.tex);
    DrawTexture(canvas.tex, 0, 0, WHITE);
  EndShaderMode();

  switch (mode) {
    case DRAWING:
      DrawTextureEx(brush.tex,
                    Vector2{ (float)(GetMouseX() - brush.tex.width  / 2 * brush.scale),
                             (float)(GetMouseY() - brush.tex.height / 2 * brush.scale) },
                    0.0,
                    brush.scale,
                    Color{ 0, 0, 0, 128} );
      break;
    case LIGHTING:
      for (int i = 0; i < lights.size(); i++) {
        DrawCircleLinesV(lights[i].position, lights[i].radius / 64 * 2, GREEN);
       }
      DrawCircleLines(GetMouseX(), GetMouseY(), brush.scale*500, brush.color);
      break;
  }

}

void Game::renderUI() {
  if (skipUIRendering) return;

  ImGui::SetNextWindowPos(ImVec2{4, 4});
  ImGui::SetNextWindowSize(ImVec2{280, 109});

  std::string str = "Drawing";
  if (mode == LIGHTING)     str = "Lighting";
  else if (mode == VIEWING) str = "Viewing";

  if (!ImGui::Begin("Mode", &debugWindowData.open, debugWindowData.flags)) {
    ImGui::End();
  } else {
    ImGui::SeparatorText(str.c_str());
    switch (mode) {
      case DRAWING: {
          if (ImGui::SmallButton("(r)eload canvas")) reloadCanvas();
          ImGui::SameLine();
          if (ImGui::SmallButton("(c)lear canvas")) clearCanvas();
          ImGui::Combo("canvas", &currentMap, "maze.png\0trees.png\0", 2);
          ImGui::SliderFloat("brush size", &brush.scale, 0.1f, 1.0f, "brush size = %.2f");
        }
        break;
      case LIGHTING: {
          if (ImGui::SmallButton("(r)eload lights")) reloadCanvas();
          ImGui::SameLine();
          if (ImGui::SmallButton("(c)lear lights")) clearCanvas();

          ImGui::SliderFloat("light size", &brush.scale, 0.1f, 1.0f, "light size = %.2f");

          Vector4 col4 = ColorNormalize(brush.color);
          float col[3] = { col4.x, col4.y, col4.z };
          ImGui::ColorEdit3("light color", col);
          brush.color = ColorFromNormalized(Vector4{col[0], col[1], col[2], 1.0});

          if (ImGui::SmallButton("toggle random colour")) randomColor = true;
        }
        break;
      case VIEWING:
        ImGui::Checkbox("\"perspective\" mode", &viewing);

        ImGui::Text("(F1) to toggle hiding UI");
        break;
    }
    ImGui::End();
  }

  if (debug) {
    ImGui::SetNextWindowPos(ImVec2{4, 136});
    ImGui::SetNextWindowSize(ImVec2{150, 142});

    if (!ImGui::Begin("Debug", &debugWindowData.open, debugWindowData.flags)) {
      ImGui::End();
    } else {
      ImGui::Text("%d FPS", GetFPS());
      ImGui::Text("cascade amount: %i", cascadeAmount);
      ImGui::SliderInt("##cascade amount", &cascadeAmount, 1, 2048, "%");

      rlImGuiImageSize(&canvas.tex, 140, 70);
      ImGui::End();
    }
  }

  DrawTextureEx(cursor.tex,
                Vector2{ (float)(GetMouseX() - cursor.img.width / 2 * CURSOR_SIZE),
                         (float)(GetMouseY() - cursor.img.height/ 2 * CURSOR_SIZE) },
                0.0,
                CURSOR_SIZE,
                WHITE);
}

void Game::processKeyboardInput() {
  if (IsKeyPressed(KEY_ONE))   mode = DRAWING;
  if (IsKeyPressed(KEY_TWO))   mode = LIGHTING;
  if (IsKeyPressed(KEY_THREE)) mode = VIEWING;

  if (IsKeyPressed(KEY_GRAVE)) debug = !debug;
  if (IsKeyPressed(KEY_F1))    skipUIRendering = !skipUIRendering;
  if (IsKeyPressed(KEY_F12)) {
    printf("Taking screenshot.\n");
    if (!DirectoryExists("screenshots")) MakeDirectory("screenshots");
    TakeScreenshot("screenshots/screenshot.png");
  }

  if (IsKeyPressed(KEY_R)) reloadCanvas();
  if (IsKeyPressed(KEY_C)) clearCanvas();
}

void Game::processMouseInput() {
  if (ImGui::GetIO().WantCaptureMouse) return;

  if (IsKeyDown(KEY_LEFT_CONTROL) && debug) {
    int amp = 50;
    if (IsKeyDown(KEY_LEFT_SHIFT)) amp = 1;
    cascadeAmount += GetMouseWheelMove() * amp;
    if (cascadeAmount < 1) cascadeAmount = 1;
  } else {
    brush.scale += GetMouseWheelMove() / 100;
  }

  if (IsMouseButtonDown(2)) {
    for (int i = 0; i < lights.size(); i++) {
      if (Vector2Length(lights[i].position - MOUSE_VECTOR) < 16) {
        lights[i].position = Vector2{ static_cast<float>(GetMouseX()), static_cast<float>(GetMouseY()) };
      }
    }
  }

  switch (mode) {
    case DRAWING:
      if (IsMouseButtonDown(0)) {
        // draw
        ImageDraw(&canvas.img,
                  brush.img,
                  Rectangle{ 0, 0, (float)canvas.img.width, (float)canvas.img.height },
                  Rectangle{ static_cast<float>(GetMouseX() - brush.img.width  / 2 * brush.scale),
                             static_cast<float>(GetMouseY() - brush.img.height / 2 * brush.scale),
                             static_cast<float>(brush.img.width  * brush.scale),
                             static_cast<float>(brush.img.height * brush.scale) },
                  BLACK);
        RELOAD_CANVAS();
      } else if (IsMouseButtonDown(1)) {
        // erase
        ImageDraw(&canvas.img,
                  brush.img,
                  Rectangle{ 0, 0, (float)canvas.img.width, (float)canvas.img.height },
                  Rectangle{ static_cast<float>(GetMouseX() - brush.img.width  / 2 * brush.scale),
                             static_cast<float>(GetMouseY() - brush.img.height / 2 * brush.scale),
                             static_cast<float>(brush.img.width  * brush.scale),
                             static_cast<float>(brush.img.height * brush.scale) },
                  WHITE);
        RELOAD_CANVAS();
      }
      break;
    case LIGHTING:
      if (IsMouseButtonPressed(0)) {
        // place light
        Light l;
        l.position = MOUSE_VECTOR;
        Vector4 col = ColorNormalize(brush.color);
        l.color    = Vector3{col.x, col.y, col.z};
        l.radius   = brush.scale * 500;
        lights.push_back(l);
      } else if (IsMouseButtonDown(1)) {
        // delete lights
        for (int i = 0; i < lights.size(); i++) {
          if (Vector2Length(lights[i].position - MOUSE_VECTOR) < 16) {
            lights.erase(lights.begin() + i);
          }
        }
      }
    break;
  }
}

void Game::reloadCanvas() {
  if (IsKeyDown(KEY_LEFT_CONTROL)) {
    // reloading
      printf("Reloading shaders.\n");
      UnloadShader(lightingShader);
      lightingShader = LoadShader(0, "res/shaders/lighting.frag");
      if (!IsShaderValid(lightingShader)) {
        UnloadShader(lightingShader);
        lightingShader = LoadShader(0, "res/shaders/broken.frag");
      }
  } else if (mode == DRAWING) {
    printf("Replacing canvas.\n");
    PRINT(maps[currentMap]);
    std::string filepath = "res/textures/canvas/" + maps[currentMap];
    canvas.img = LoadImage(filepath.c_str());
    RELOAD_CANVAS();
  } else if (mode == LIGHTING) {
    printf("Replacing lights.\n");
    lights.clear();
    placeLights(&lights);
  }
}

void Game::clearCanvas() {
  switch (mode) {
    case DRAWING:
      printf("Clearing canvas.\n");
      canvas.img = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
      RELOAD_CANVAS();
      break;
    case LIGHTING:
      printf("Clearing lights.\n");
      lights.clear();
      break;
  }
}
