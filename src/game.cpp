#include "game.h"

#define MOUSE_VECTOR Vector2{ static_cast<float>(GetMouseX()), static_cast<float>(GetMouseY()) }
#define RELOAD_CANVAS() UnloadTexture(canvas.tex); \
                        canvas.tex = LoadTextureFromImage(canvas.img);
#define RANDOM_COLOR ColorFromNormalized(Vector4{ (std::sin(time) + 1) / 2, (std::cos(time) + 1) / 2, (std::sin(time*2) + 1) / 2, 1.0 })

Game::Game() {
  debug = false;
  mode  = DRAWING;
  skipUIRendering = false;
  randomLightColor = false;
  randomLightSize  = false;
  perspective = false;

  lightingShader = LoadShader(0, "res/shaders/lighting.frag");
  if (!IsShaderValid(lightingShader)) {
    printf("lightingShader is broken!!\n");
    UnloadShader(lightingShader);
    lightingShader = LoadShader(0, "res/shaders/broken.frag");
  }

  cascadeAmount = 512;

  brush.lightSize = 128.0;
  brush.lightType = 0;

  cursor.img = LoadImage("res/textures/cursor.png");
  cursor.tex = LoadTextureFromImage(cursor.img);

  brush.img = LoadImage("res/textures/brush.png");
  brush.tex = LoadTextureFromImage(brush.img);
  brush.brushSize = 0.25;

  std::string filepath = "res/textures/canvas/" + maps[currentMap];
  canvas.img = LoadImage(filepath.c_str());
  canvas.tex = LoadTextureFromImage(canvas.img);

  placeLights();

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

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Game::update() {
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

  int v = perspective;
  if (mode != VIEWING) v = 0;
  SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uViewing"), &v, SHADER_UNIFORM_INT);

  for (int i = 0; i < lights.size(); i++) {
    Vector2 position = lights[i].position * GetWindowScaleDPI();
    float   radius   = lights[i].radius * GetWindowScaleDPI().x;
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, TextFormat("lights[%i].position",    i)), &position,              SHADER_UNIFORM_VEC2);
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, TextFormat("lights[%i].color",       i)), &lights[i].color,       SHADER_UNIFORM_VEC3);
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, TextFormat("lights[%i].radius",      i)), &radius,                SHADER_UNIFORM_FLOAT);
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, TextFormat("lights[%i].timeCreated", i)), &lights[i].timeCreated, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, TextFormat("lights[%i].type", i)),        &lights[i].type,        SHADER_UNIFORM_INT);
   }

  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureMouse) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
  } else {
    ImGui::SetMouseCursor(ImGuiMouseCursor_None);
  }

  if (randomLightColor) brush.lightColor = RANDOM_COLOR;
  if (randomLightSize)  brush.lightSize  = MIN_LIGHT_SIZE * 4 + std::abs(std::sin(time) * (MAX_LIGHT_SIZE/2 - MIN_LIGHT_SIZE));
  if (randomLightType)  brush.lightType = static_cast<int>(std::abs(std::sin(time)) * 3);
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

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
                    Vector2{ (float)(GetMouseX() - brush.tex.width  / 2 * brush.brushSize),
                             (float)(GetMouseY() - brush.tex.height / 2 * brush.brushSize) },
                    0.0,
                    brush.brushSize,
                    Color{ 0, 0, 0, 128} );
      break;
    case LIGHTING:
      for (int i = 0; i < lights.size(); i++) {
        DrawCircleLinesV(lights[i].position, lights[i].radius / 64 * 2, GREEN);
       }
      DrawCircleLines(GetMouseX(), GetMouseY(), brush.lightSize, brush.lightColor);
      break;
  }

}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Game::renderUI() {
  if (skipUIRendering) return;

  float h = 100;
  if (mode == LIGHTING) h = 170;
  if (mode == VIEWING)  h = 70;
  if (debug) h += 40;

  ImGui::SetNextWindowSize(ImVec2{300, h});
  ImGui::SetNextWindowPos(ImVec2{4, 4});

  std::string str = "Drawing";
  if (mode == LIGHTING)     str = "Lighting";
  else if (mode == VIEWING) str = "Viewing";

  // ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4{0.2, 0.2, 0.2, 0.15});
  // ImGui::PushStyleColor(ImGuiCol_Border, ImVec4{0.2, 0.2, 0.2, 0.15});

  if (!ImGui::Begin("Mode", &debugWindowData.open, debugWindowData.flags)) {
    ImGui::End();
  } else {
    if (debug) {
      ImGui::Text("%d FPS", GetFPS());
      ImGui::SliderInt("##cascade amount", &cascadeAmount, 1, 2048, "cascade amount = %i");
    }
    ImGui::SeparatorText(str.c_str());
    switch (mode) {
      case DRAWING: {
          if (ImGui::SmallButton("(r)eload canvas")) reloadCanvas();
          ImGui::SameLine();
          if (ImGui::SmallButton("(c)lear canvas")) clearCanvas();
          ImGui::Combo("canvas", &currentMap, "maze.png\0trees.png\0", 2);
          ImGui::SliderFloat("brush size", &brush.brushSize, 0.1f, 1.0f, "brush size = %.2f");
        }
        break;
      case LIGHTING: {
          if (ImGui::SmallButton("(r)eload lights")) reloadCanvas();
          ImGui::SameLine();
          if (ImGui::SmallButton("(c)lear lights")) clearCanvas();

          ImGui::SliderFloat("light size", &brush.lightSize, MIN_LIGHT_SIZE, MAX_LIGHT_SIZE, "light size = %.0fpx");

          ImGui::Combo("light type", &brush.lightType, "static\0sine\0flickering\0", 3);

          if (ImGui::SmallButton("set random colour"))  brush.lightColor = RANDOM_COLOR;
          Vector4 col4 = ColorNormalize(brush.lightColor);
          float col[3] = { col4.x, col4.y, col4.z };
          ImGui::ColorEdit3("light color", col);
          brush.lightColor = ColorFromNormalized(Vector4{col[0], col[1], col[2], 1.0});

          ImGui::Text("toggle random...");
          if (ImGui::SmallButton("type")) randomLightType = !randomLightType;
          ImGui::SameLine();
          if (ImGui::SmallButton("colour")) randomLightColor = !randomLightColor;
          ImGui::SameLine();
          if (ImGui::SmallButton("size")) randomLightSize  = !randomLightSize;
        }
        break;
      case VIEWING:
        ImGui::Checkbox("\"perspective\" mode", &perspective);

        ImGui::Text("(F1) to toggle hiding UI");
        break;
    }
    ImGui::End();
  }

  // ImGui::PopStyleColor();
  // ImGui::PopStyleColor();

  // if (debug) {
  //   ImGui::SetNextWindowPos(ImVec2{4, 136});
  //   ImGui::SetNextWindowSize(ImVec2{150, 142});
  //
  //   if (!ImGui::Begin("Debug", &debugWindowData.open, debugWindowData.flags)) {
  //     ImGui::End();
  //   } else {
  //     ImGui::Text("%d FPS", GetFPS());
  //     ImGui::Text("cascade amount: %i", cascadeAmount);
  //     ImGui::SliderInt("##cascade amount", &cascadeAmount, 1, 2048, "%");
  //
  //     rlImGuiImageSize(&canvas.tex, 140, 70);
  //     ImGui::End();
  //   }
  // }

  DrawTextureEx(cursor.tex,
                Vector2{ (float)(GetMouseX() - cursor.img.width / 2 * CURSOR_SIZE),
                         (float)(GetMouseY() - cursor.img.height/ 2 * CURSOR_SIZE) },
                0.0,
                CURSOR_SIZE,
                WHITE);
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// FIX: screenshots arent saved in a specified directory, might need to adapt raylib's screenshot function
void Game::processKeyboardInput() {
  if (IsKeyPressed(KEY_ONE))   mode = DRAWING;
  if (IsKeyPressed(KEY_TWO))   mode = LIGHTING;
  if (IsKeyPressed(KEY_THREE)) mode = VIEWING;

  if (IsKeyPressed(KEY_GRAVE)) debug = !debug;
  if (IsKeyPressed(KEY_F1))    skipUIRendering = !skipUIRendering;
  // if (IsKeyPressed(KEY_F12)) {
  //   printf("Taking screenshot.\n");
  //   if (!DirectoryExists("screenshots")) MakeDirectory("screenshots");
  //   std::string pwd = GetWorkingDirectory();
  //   ChangeDirectory("screenshots");
  //   TakeScreenshot("screenshot.png");
  //   ChangeDirectory(pwd.c_str());
  // }

  if (IsKeyPressed(KEY_R)) reloadCanvas();
  if (IsKeyPressed(KEY_C)) clearCanvas();
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Game::processMouseInput() {
  if (ImGui::GetIO().WantCaptureMouse) return;

  if      (mode == DRAWING)  brush.brushSize += GetMouseWheelMove() / 100;
  else if (mode == LIGHTING) brush.lightSize += GetMouseWheelMove() * 3;

  if      (brush.brushSize < 0.1) brush.brushSize = 0.1;
  else if (brush.brushSize > 1.0) brush.brushSize = 1.0;

  if      (brush.lightSize < MIN_LIGHT_SIZE) brush.lightSize = MIN_LIGHT_SIZE;
  else if (brush.lightSize > MAX_LIGHT_SIZE) brush.lightSize = MAX_LIGHT_SIZE;


  if (IsMouseButtonDown(2)) {
    for (int i = 0; i < lights.size(); i++) {
      if (Vector2Length(lights[i].position - MOUSE_VECTOR) < 32) {
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
                  Rectangle{ static_cast<float>(GetMouseX() - brush.img.width  / 2 * brush.brushSize),
                             static_cast<float>(GetMouseY() - brush.img.height / 2 * brush.brushSize),
                             static_cast<float>(brush.img.width  * brush.brushSize),
                             static_cast<float>(brush.img.height * brush.brushSize) },
                  BLACK);
        RELOAD_CANVAS();
      } else if (IsMouseButtonDown(1)) {
        // erase
        ImageDraw(&canvas.img,
                  brush.img,
                  Rectangle{ 0, 0, (float)canvas.img.width, (float)canvas.img.height },
                  Rectangle{ static_cast<float>(GetMouseX() - brush.img.width  / 2 * brush.brushSize),
                             static_cast<float>(GetMouseY() - brush.img.height / 2 * brush.brushSize),
                             static_cast<float>(brush.img.width  * brush.brushSize),
                             static_cast<float>(brush.img.height * brush.brushSize) },
                  WHITE);
        RELOAD_CANVAS();
      }
      break;
    case LIGHTING:
      if (IsMouseButtonPressed(0)) {
        // place light
        Vector4 col = ColorNormalize(brush.lightColor);
        Vector3 color = Vector3{col.x, col.y, col.z};
        addLight(MOUSE_VECTOR, color, brush.lightSize, (LightType)brush.lightType);
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

void Game::addLight(Vector2 position, Vector3 normalisedColor, float radius, LightType type) {
  Light l;
  l.position    = position;
  l.color       = normalisedColor;
  l.radius      = radius;
  l.timeCreated = GetTime();
  l.type        = type;
  lights.push_back(l);
}

void Game::placeLights(int lightNumber, float distFromCentre) {
  for (float i = 0; i < lightNumber; i++) {
    float t = (i+1) * (PI*2/lightNumber) + 0.1;
    addLight(
      Vector2{ SCREEN_WIDTH/2 + std::sin(t) * distFromCentre, SCREEN_HEIGHT/2 + std::cos(t) * distFromCentre},
      Vector3{ std::sin(t), std::cos(t), 1.0 },
      300,
      STATIC
    );
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
    placeLights();
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
