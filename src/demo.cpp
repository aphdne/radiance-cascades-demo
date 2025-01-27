#include "demo.h"

#define MOUSE_VECTOR Vector2{ static_cast<float>(GetMouseX()), static_cast<float>(GetMouseY()) }
#define RELOAD_CANVAS() UnloadTexture(canvas.tex); \
                        canvas.tex = LoadTextureFromImage(canvas.img);
#define RANDOM_COLOR ColorFromNormalized(Vector4{\
                                              (std::sin(static_cast<float>(GetTime()))   + 1) / 2,\
                                              (std::cos(static_cast<float>(GetTime()))   + 1) / 2,\
                                              (std::sin(static_cast<float>(GetTime())*2) + 1) / 2,\
                                              1.0 })

Demo::Demo() {
  // user parameters
  user.mode  = DRAWING;
  perspective = false;
  skipUIRendering = false;

  user.lightSize = 128.0;
  user.lightType = 0; // STATIC
  user.lightColor = RANDOM_COLOR;

  randomLightColor = false;
  randomLightSize  = false;
  randomLightType  = false;
  timeSinceLastType = GetTime(); // for randomLightType cycling

  // ui parameters

  debug = false;
  help  = true;

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

  // resource loading

  cursor.img = LoadImage("res/textures/cursor.png");
  cursor.tex = LoadTextureFromImage(cursor.img);

  user.brush.img = LoadImage("res/textures/brush.png");

  for (int x = 0; x < user.brush.img.width; x++) {
    for (int y = 0; y < user.brush.img.height; y++) {
      if (GetImageColor(user.brush.img, x, y).a <= 0.25) {
        ImageDrawPixel(&user.brush.img, x, y, Color{0, 0, 0, 0});
      } else {
        ImageDrawPixel(&user.brush.img, x, y, WHITE);
      }
    }
  }

  user.brush.tex = LoadTextureFromImage(user.brush.img);
  user.brushSize = 0.25;

  std::string filepath = "res/textures/canvas/" + maps[currentMap];
  canvas.img = LoadImage(filepath.c_str());
  canvas.tex = LoadTextureFromImage(canvas.img);

  loadShader("rc.frag");
  loadShader("jfa.frag");
  loadShader("prep.frag");

  // misc

  HideCursor();
  cascadeAmount = 512;

}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Demo::update() {
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Demo::render() {
  ClearBackground(PINK);

  rcShader = shaders["rc.frag"];
  jfaShader = shaders["jfa.frag"];
  prepShader = shaders["prep.frag"];

  RenderTexture2D bufferA = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
  RenderTexture2D bufferB = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
  RenderTexture2D bufferC = bufferA;

  // for shader uniforms
  Vector2 resolution = { SCREEN_WIDTH, SCREEN_HEIGHT };
  Vector2 dpi = GetWindowScaleDPI();
  // resolution *= GetWindowScaleDPI();
  // PRINT(GetWindowScaleDPI().x);
  // PRINT(GetWindowScaleDPI().y);

  SetTextureWrap(canvas.tex, TEXTURE_WRAP_CLAMP);

  // first render pass for JFA
  // create UV mask w/ prep shader
  BeginTextureMode(bufferA);
    BeginShaderMode(prepShader);
      SetShaderValueTexture(prepShader, GetShaderLocation(prepShader, "uCanvas"), canvas.tex);
      SetShaderValue(prepShader, GetShaderLocation(prepShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);
      SetShaderValue(prepShader, GetShaderLocation(prepShader, "uDPI"), &dpi, SHADER_UNIFORM_VEC2);
      DrawTexture(canvas.tex, 0, 0, WHITE);
    EndShaderMode();
  EndTextureMode();

  for (int j = cascadeAmount; j >= 1; j /= 2) {
    bufferC = bufferA;
    bufferA = bufferB;
    bufferB = bufferC;

    BeginTextureMode(bufferA);
      BeginShaderMode(jfaShader);
        SetShaderValueTexture(jfaShader, GetShaderLocation(jfaShader, "uCanvas"), bufferB.texture);
        SetShaderValue(jfaShader, GetShaderLocation(jfaShader, "uJumpSize"), &j, SHADER_UNIFORM_INT);
        SetShaderValue(jfaShader, GetShaderLocation(jfaShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);
        DrawTexture(bufferB.texture, 0, 0, WHITE);
      EndShaderMode();
    EndTextureMode();
  }

  // // display bufferA to main framebuffer
  BeginShaderMode(rcShader);
    SetShaderValue(rcShader, GetShaderLocation(rcShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(rcShader, GetShaderLocation(rcShader, "uDPI"), &dpi, SHADER_UNIFORM_VEC2);
    // SetShaderValueTexture(rcShader, GetShaderLocation(rcShader, "uCanvas"), canvas.tex);
    // DrawTexture(canvas.tex, 0, 0, WHITE);
    SetShaderValueTexture(rcShader, GetShaderLocation(rcShader, "uCanvas"), bufferA.texture);
    DrawTexture(bufferA.texture, 0, 0, WHITE);
  EndShaderMode();

  // switch (user.mode) {
  //   case DRAWING:
      DrawTextureEx(user.brush.tex,
                    Vector2{ (float)(GetMouseX() - user.brush.tex.width  / 2 * user.brushSize),
                             (float)(GetMouseY() - user.brush.tex.height / 2 * user.brushSize) },
                    0.0,
                    user.brushSize,
                    Color{ 0, 0, 0, 128} );
      // break;
  //   case LIGHTING:
  //     for (int i = 0; i < lights.size(); i++) {
  //       DrawCircleLinesV(lights[i].position, lights[i].radius / 64 * 2, GREEN);
  //      }
  //     DrawCircleLines(GetMouseX(), GetMouseY(), user.lightSize, user.lightColor);
  //     break;
  // }

  UnloadRenderTexture(bufferA);
  UnloadRenderTexture(bufferB);
  UnloadRenderTexture(bufferC);
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Demo::renderUI() {
  if (skipUIRendering) return;

  // float h = 100;
  // if (user.mode == LIGHTING) h = 170;
  // if (user.mode == VIEWING)  h = 70;
  // if (help)  h += 115;
  // if (debug) h += 78;
  float h = 78;

  ImGui::SetNextWindowSize(ImVec2{300, h});
  ImGui::SetNextWindowPos(ImVec2{4, 4});

  std::string str = "Drawing";
  if (user.mode == LIGHTING)     str = "Lighting";
  else if (user.mode == VIEWING) str = "Viewing";

  if (!ImGui::Begin("Mode", &debugWindowData.open, debugWindowData.flags)) {
    ImGui::End();
  } else {
    if (debug) {
      ImGui::SeparatorText("Debug");
      ImGui::Text("%d FPS", GetFPS());
      ImGui::SliderInt("##cascade amount", &cascadeAmount, 0, 1024, "cascade amount = %i");
      Vector4 c = ColorNormalize(user.lightColor);
      ImGui::Text("light seed = %f", (c.x + c.y + c.z) / 1.5 + 1);
    }

    // if (help) {
    //   ImGui::SeparatorText("Help");
    //   ImGui::Text("a basic 2D lighting demo\n");
    //   ImGui::Spacing();
    //   ImGui::Text("press 1, 2 or 3 to toggle between \ndrawing, lighting, and viewing mode");
    //   ImGui::Spacing();
    //   ImGui::Text("press (`) for sneaky debug controls/info");
    //   ImGui::Text("press (h) to hide this text");
    // }
    //
    // ImGui::SeparatorText(str.c_str());
    // switch (user.mode) {
    //   case DRAWING: {
    //       if (ImGui::SmallButton("(r)eload canvas")) reload();
    //       ImGui::SameLine();
    //       if (ImGui::SmallButton("(c)lear canvas")) clear();
    //       ImGui::Combo("canvas", &currentMap, "maze.png\0trees.png\0", 2);
    //       ImGui::SliderFloat("brush size", &user.brushSize, 0.1f, 1.0f, "brush size = %.2f");
    //     }
    //     break;
    //   case LIGHTING: {
    //       if (ImGui::SmallButton("(r)eload lights")) reload();
    //       ImGui::SameLine();
    //       if (ImGui::SmallButton("(c)lear lights")) clear();
    //
    //       ImGui::SliderFloat("light size", &user.lightSize, MIN_LIGHT_SIZE, MAX_LIGHT_SIZE, "light size = %.0fpx");
    //
    //       ImGui::Combo("light type", &user.lightType, "static\0sine\0saw\0noise\0", 3);
    //
    //       if (ImGui::SmallButton("set random colour"))  user.lightColor = RANDOM_COLOR;
    //       Vector4 col4 = ColorNormalize(user.lightColor);
    //       float col[3] = { col4.x, col4.y, col4.z };
    //       ImGui::ColorEdit3("light color", col);
    //       user.lightColor = ColorFromNormalized(Vector4{col[0], col[1], col[2], 1.0});
    //
    //       ImGui::Text("toggle random...");
    //       if (ImGui::SmallButton("type")) randomLightType = !randomLightType;
    //       ImGui::SameLine();
    //       if (ImGui::SmallButton("colour")) randomLightColor = !randomLightColor;
    //       ImGui::SameLine();
    //       if (ImGui::SmallButton("size")) randomLightSize  = !randomLightSize;
    //     }
    //     break;
    //   case VIEWING:
    //     ImGui::Checkbox("\"perspective\" mode", &perspective);
    //
    //     ImGui::Text("(F1) to toggle hiding UI");
    //     break;
    // }
    ImGui::End();
  }

  // dont draw our custom cursor if we are mousing over the UI
  ImGuiIO& io = ImGui::GetIO();
  if (!io.WantCaptureMouse) {
    DrawTextureEx(cursor.tex,
                  Vector2{ (float)(GetMouseX() - cursor.img.width / 2 * CURSOR_SIZE),
                           (float)(GetMouseY() - cursor.img.height/ 2 * CURSOR_SIZE) },
                  0.0,
                  CURSOR_SIZE,
                  WHITE);
  }

}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// FIX: screenshots arent saved in a specified directory, might need to adapt raylib's screenshot function
void Demo::processKeyboardInput() {
  if (IsKeyPressed(KEY_ONE))   user.mode = DRAWING;
  if (IsKeyPressed(KEY_TWO))   user.mode = LIGHTING;
  if (IsKeyPressed(KEY_THREE)) user.mode = VIEWING;

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

  if (IsKeyPressed(KEY_C)) clear();
  if (IsKeyPressed(KEY_R)) reload();
  if (IsKeyPressed(KEY_H)) help = !help;

  // if (IsKeyPressed(KEY_F)) ToggleFullscreen();
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Demo::processMouseInput() {
  // we do not want to be affecting the scene when we're clicking on the UI
  if (ImGui::GetIO().WantCaptureMouse) return;

  // if      (user.mode == DRAWING)  user.brushSize += GetMouseWheelMove() / 100;
  // else if (user.mode == LIGHTING) user.lightSize += GetMouseWheelMove() * 3;
  user.brushSize += GetMouseWheelMove() / 100;

  if      (user.brushSize < 0.1) user.brushSize = 0.1;
  else if (user.brushSize > 1.0) user.brushSize = 1.0;

  if      (user.lightSize < MIN_LIGHT_SIZE) user.lightSize = MIN_LIGHT_SIZE;
  else if (user.lightSize > MAX_LIGHT_SIZE) user.lightSize = MAX_LIGHT_SIZE;

  if (IsMouseButtonDown(2) || (IsKeyDown(KEY_LEFT_SHIFT) && IsMouseButtonDown(0))) {
    for (int i = 0; i < lights.size(); i++) {
      if (Vector2Length(lights[i].position - MOUSE_VECTOR) < 32) {
        lights[i].position = Vector2{ static_cast<float>(GetMouseX()), static_cast<float>(GetMouseY()) };
      }
    }
    return;
  }

  switch (user.mode) {
    case DRAWING:
      if (IsMouseButtonDown(0) && !IsKeyDown(KEY_LEFT_CONTROL)) {
        // draw
        ImageDraw(&canvas.img,
                  user.brush.img,
                  Rectangle{ 0, 0, (float)canvas.img.width, (float)canvas.img.height },
                  Rectangle{ static_cast<float>(GetMouseX() - user.brush.img.width  / 2 * user.brushSize),
                             static_cast<float>(GetMouseY() - user.brush.img.height / 2 * user.brushSize),
                             static_cast<float>(user.brush.img.width  * user.brushSize),
                             static_cast<float>(user.brush.img.height * user.brushSize) },
                  BLACK);
        RELOAD_CANVAS();
      } else if (IsMouseButtonDown(1) || (IsKeyDown(KEY_LEFT_CONTROL) && IsMouseButtonDown(0))) {
        // erase
        ImageDraw(&canvas.img,
                  user.brush.img,
                  Rectangle{ 0, 0, (float)canvas.img.width, (float)canvas.img.height },
                  Rectangle{ static_cast<float>(GetMouseX() - user.brush.img.width  / 2 * user.brushSize),
                             static_cast<float>(GetMouseY() - user.brush.img.height / 2 * user.brushSize),
                             static_cast<float>(user.brush.img.width  * user.brushSize),
                             static_cast<float>(user.brush.img.height * user.brushSize) },
                  WHITE);
        RELOAD_CANVAS();
      }
      break;
    case LIGHTING:
      if (IsMouseButtonDown(0) && !IsKeyDown(KEY_LEFT_CONTROL)) {
        // draw
        ImageDraw(&canvas.img,
                  user.brush.img,
                  Rectangle{ 0, 0, (float)canvas.img.width, (float)canvas.img.height },
                  Rectangle{ static_cast<float>(GetMouseX() - user.brush.img.width  / 2 * user.brushSize),
                             static_cast<float>(GetMouseY() - user.brush.img.height / 2 * user.brushSize),
                             static_cast<float>(user.brush.img.width  * user.brushSize),
                             static_cast<float>(user.brush.img.height * user.brushSize) },
                  user.lightColor);
        RELOAD_CANVAS();
      } else if (IsMouseButtonDown(1) || (IsKeyDown(KEY_LEFT_CONTROL) && IsMouseButtonDown(0))) {
        // erase
        ImageDraw(&canvas.img,
                  user.brush.img,
                  Rectangle{ 0, 0, (float)canvas.img.width, (float)canvas.img.height },
                  Rectangle{ static_cast<float>(GetMouseX() - user.brush.img.width  / 2 * user.brushSize),
                             static_cast<float>(GetMouseY() - user.brush.img.height / 2 * user.brushSize),
                             static_cast<float>(user.brush.img.width  * user.brushSize),
                             static_cast<float>(user.brush.img.height * user.brushSize) },
                  WHITE);
        RELOAD_CANVAS();
      }
      break;
      // if (IsMouseButtonPressed(0) && !IsKeyDown(KEY_LEFT_CONTROL)) {
      //   // place light
      //   Vector4 col = ColorNormalize(user.lightColor);
      //   Vector3 color = Vector3{col.x, col.y, col.z};
      //   addLight(MOUSE_VECTOR, color, user.lightSize, (LightType)user.lightType);
      // } else if (IsMouseButtonDown(1) || (IsKeyDown(KEY_LEFT_CONTROL) && IsMouseButtonDown(0))) {
      //   // delete lights
      //   for (int i = 0; i < lights.size(); i++) {
      //     if (Vector2Length(lights[i].position - MOUSE_VECTOR) < 16) {
      //       lights.erase(lights.begin() + i);
      //     }
      //   }
      // }
    break;
  }
}

void Demo::loadShader(std::string shader) {
  std::string path = "res/shaders/" + shader;
  Shader s = LoadShader(0, path.c_str());
  if (!IsShaderValid(s)) {
    std::cout << "ERR: '" << shader << "' is broken." << std::endl;
    UnloadShader(s);
    s = LoadShader(0, "res/shaders/broken.frag");
  } else {
    std::cout << "Loaded '" << shader << "' successfully." << std::endl;
  }
  shaders[shader] = s;
}

void Demo::reloadShaders() {
  std::cout << "Reloading shaders." << std::endl;
  for (auto const& [key, val] : shaders) {
    loadShader(key);
  }
}

void Demo::addLight(Vector2 position, Vector3 normalisedColor, float radius, LightType type) {
  Light l;
  l.position    = position;
  l.color       = normalisedColor;
  l.radius      = radius;
  l.timeCreated = GetTime();
  l.type        = type;
  lights.push_back(l);
}

// place lights along an arbitray circle
void Demo::placeLights(int lightNumber, float distFromCentre) {
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

// reload based on what mode we're in, unless we're press control
// if we're pressing control we reload shaders
//
// DRAWING: load canvas image texture
// LIGHTING: set lighting to how it is when the programme starts
void Demo::reload() {
  if (IsKeyDown(KEY_LEFT_CONTROL)) {
    reloadShaders();
  } else if (user.mode == DRAWING) {
    printf("Replacing canvas.\n");
    std::string filepath = "res/textures/canvas/" + maps[currentMap];
    canvas.img = LoadImage(filepath.c_str());
    RELOAD_CANVAS();
  } else if (user.mode == LIGHTING) {
    printf("Replacing lights.\n");
    lights.clear();
    placeLights();
  }
}

// clear depending on what mode we're in
//
// DRAWING: clear canvas
// LIGHTING: clear lights
//
void Demo::clear() {
  switch (user.mode) {
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
