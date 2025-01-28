#include "demo.h"

#define MOUSE_VECTOR Vector2{ static_cast<float>(GetMouseX()), static_cast<float>(GetMouseY()) }
#define RELOAD_CANVAS() UnloadTexture(occlusionMap.tex); \
                        occlusionMap.tex = LoadTextureFromImage(occlusionMap.img); \
                        UnloadTexture(emitterMap.tex); \
                        emitterMap.tex = LoadTextureFromImage(emitterMap.img);
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

  user.lightColor = RANDOM_COLOR;

  // ui parameters

  debug = false;
  help  = true;

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
  occlusionMap.img = LoadImage(filepath.c_str());
  occlusionMap.tex = LoadTextureFromImage(occlusionMap.img);
  emitterMap.img = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
  emitterMap.tex = LoadTextureFromImage(emitterMap.img);

  // automatically load fragment shaders in the res/shaders directory
  FilePathList shaderFiles = LoadDirectoryFilesEx("res/shaders", ".frag", false);
  for (int i = 0; i < shaderFiles.count; i++) {
    std::string str = shaderFiles.paths[i];
    str.erase(0, 12);
    if (str == "broken.frag") continue;
    loadShader(str);
  }
  UnloadDirectoryFiles(shaderFiles);

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

  Shader& lightingShader = shaders["gi.frag"];
  Shader& jfaShader = shaders["jfa.frag"];
  Shader& prepShader = shaders["prep.frag"];

  RenderTexture2D bufferA = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
  RenderTexture2D bufferB = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
  RenderTexture2D bufferC = bufferA;

  // for shader uniforms
  Vector2 resolution = { SCREEN_WIDTH, SCREEN_HEIGHT };

  // first render pass for JFA
  // create UV mask w/ prep shader
  BeginTextureMode(bufferA);
    BeginShaderMode(prepShader);
      SetShaderValueTexture(prepShader, GetShaderLocation(prepShader, "uOcclusionMap"), occlusionMap.tex);
      SetShaderValueTexture(prepShader, GetShaderLocation(prepShader, "uEmitterMap"),   emitterMap.tex);
      SetShaderValue(prepShader, GetShaderLocation(prepShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);
      DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    EndShaderMode();
  EndTextureMode();

  // ping-pong buffering
  // alternate between two buffers so that we can implement a recursive shader
  // see https://mini.gmshaders.com/p/gm-shaders-mini-recursive-shaders-1308459
  for (int j = cascadeAmount; j >= 1; j /= 2) {
    bufferC = bufferA;
    bufferA = bufferB;
    bufferB = bufferC;

    BeginTextureMode(bufferA);
      BeginShaderMode(jfaShader);
        SetShaderValueTexture(jfaShader, GetShaderLocation(jfaShader, "uCanvas"), bufferB.texture);
        SetShaderValue(jfaShader, GetShaderLocation(jfaShader, "uJumpSize"), &j, SHADER_UNIFORM_INT);
        SetShaderValue(jfaShader, GetShaderLocation(jfaShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
      EndShaderMode();
    EndTextureMode();
  }

  resolution *= GetWindowScaleDPI();

  // // display bufferA to main framebuffer
  BeginShaderMode(lightingShader);
    int maxSteps  = 128;
    int raysPerPx = 32;
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uRaysPerPx"),  &raysPerPx,  SHADER_UNIFORM_INT);
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uMaxSteps"),   &maxSteps,   SHADER_UNIFORM_INT);
    SetShaderValueTexture(lightingShader, GetShaderLocation(lightingShader, "uDistanceField"), bufferA.texture);
    SetShaderValueTexture(lightingShader, GetShaderLocation(lightingShader, "uOcclusionMap"),  occlusionMap.tex);
    SetShaderValueTexture(lightingShader, GetShaderLocation(lightingShader, "uEmitterMap"),    emitterMap.tex);
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  EndShaderMode();

  UnloadRenderTexture(bufferA);
  UnloadRenderTexture(bufferB);
  UnloadRenderTexture(bufferC);

  DrawTextureEx(user.brush.tex,
                Vector2{ (float)(GetMouseX() - user.brush.tex.width  / 2 * user.brushSize),
                         (float)(GetMouseY() - user.brush.tex.height / 2 * user.brushSize) },
                0.0,
                user.brushSize,
                Color{ 0, 0, 0, 128} );
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
  // else if (user.mode == VIEWING) str = "Viewing";

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
  // if (IsKeyPressed(KEY_THREE)) user.mode = VIEWING;

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

// TODO: hardware-accelerate brush drawing
void Demo::processMouseInput() {
  // we do not want to be affecting the scene when we're clicking on the UI
  if (ImGui::GetIO().WantCaptureMouse) return;

  user.brushSize += GetMouseWheelMove() / 100;

  if      (user.brushSize < 0.1) user.brushSize = 0.1;
  else if (user.brushSize > 1.0) user.brushSize = 1.0;

  ImageTexture canvas = occlusionMap;
  if (user.mode == LIGHTING) canvas = emitterMap;

  if (IsMouseButtonDown(0) && !IsKeyDown(KEY_LEFT_CONTROL)) {
    // draw
    ImageDraw(&canvas.img,
              user.brush.img,
              Rectangle{ 0, 0, (float)canvas.img.width, (float)canvas.img.height },
              Rectangle{ static_cast<float>(GetMouseX() - user.brush.img.width  / 2 * user.brushSize),
                         static_cast<float>(GetMouseY() - user.brush.img.height / 2 * user.brushSize),
                         static_cast<float>(user.brush.img.width  * user.brushSize),
                         static_cast<float>(user.brush.img.height * user.brushSize) },
              (user.mode == LIGHTING) ? user.lightColor : BLACK);
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

// reload based on what mode we're in, unless we're press control
// if we're pressing control we reload shaders
//
// DRAWING: load canvas image texture
// LIGHTING: set lighting to how it is when the programme starts
void Demo::reload() {
  if (IsKeyDown(KEY_LEFT_CONTROL)) {
    reloadShaders();
  } else {
    printf("Replacing canvas.\n");
    std::string filepath = "res/textures/canvas/" + maps[currentMap];
    occlusionMap.img = LoadImage(filepath.c_str());
    emitterMap.img = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
    RELOAD_CANVAS();
  }
}

// clear depending on what mode we're in
//
// DRAWING: clear canvas
// LIGHTING: clear lights
//
void Demo::clear() {
  printf("Clearing canvas.\n");
  occlusionMap.img = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  emitterMap.img = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
  RELOAD_CANVAS();
}
