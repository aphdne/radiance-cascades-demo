#include "demo.h"

#define MOUSE_VECTOR Vector2{ static_cast<float>(GetMouseX()), static_cast<float>(GetMouseY()) }
#define RELOAD_CANVAS() UnloadTexture(occlusionMap.tex); \
                        occlusionMap.tex = LoadTextureFromImage(occlusionMap.img); \
                        UnloadTexture(emissionMap.tex); \
                        emissionMap.tex = LoadTextureFromImage(emissionMap.img);
#define RANDOM_COLOR ColorFromNormalized(Vector4{\
                                              (std::sin(static_cast<float>(GetTime()))   + 1) / 2,\
                                              (std::cos(static_cast<float>(GetTime()))   + 1) / 2,\
                                              (std::sin(static_cast<float>(GetTime())*2) + 1) / 2,\
                                              1.0 })

Demo::Demo() {
  // user parameters

  user.mode  = DRAWING;
  skipUIRendering = false;
  maxSteps = 64;
  raysPerPx = 256;

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

  // for shader uniforms
  resolution = { SCREEN_WIDTH, SCREEN_HEIGHT };

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
  emissionMap.img = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
  emissionMap.tex = LoadTextureFromImage(emissionMap.img);

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

  Shader& lightingShader  = shaders["gi.frag"];
  Shader& jfaShader       = shaders["jfa.frag"];
  Shader& prepJfaShader   = shaders["prepjfa.frag"];
  Shader& scenePrepShader = shaders["prepscene.frag"];
  Shader& distFieldShader = shaders["distfield.frag"];

  // loading FBOs outside of the render loop seems to break this entire setup, not sure why
  RenderTexture2D sceneBuf     = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
  RenderTexture2D bufferA      = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
  RenderTexture2D bufferB      = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
  RenderTexture2D bufferC      = bufferA;
  RenderTexture2D distFieldBuf = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);

  // change bit depth for bufferA, B, & C so that we can encode texture coordinates without losing data
  // default Raylib FBOs have a bit depth of 8 per channel, which would only cover for a window of maximum size 255x255
  // we can also save some memory by reducing bit depth of buffers to what is strictly required
  auto changeBitDepth = [](RenderTexture2D &buffer, PixelFormat pixformat) {
    rlEnableFramebuffer(buffer.id);
      rlUnloadTexture(buffer.texture.id);
      buffer.texture.id = rlLoadTexture(NULL, SCREEN_WIDTH, SCREEN_HEIGHT, pixformat, 1);
      buffer.texture.width = SCREEN_WIDTH;
      buffer.texture.height = SCREEN_HEIGHT;
      buffer.texture.format = pixformat;
      buffer.texture.mipmaps = 1;
      rlFramebufferAttach(buffer.id, buffer.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
    rlDisableFramebuffer();
  };

  changeBitDepth(bufferA, PIXELFORMAT_UNCOMPRESSED_R16G16B16A16);
  changeBitDepth(bufferB, PIXELFORMAT_UNCOMPRESSED_R16G16B16A16);
  changeBitDepth(bufferC, PIXELFORMAT_UNCOMPRESSED_R16G16B16A16);
  changeBitDepth(sceneBuf, PIXELFORMAT_UNCOMPRESSED_R5G5B5A1);
  changeBitDepth(distFieldBuf, PIXELFORMAT_UNCOMPRESSED_R16);

  // create scene texture - combining emission & occlusion maps into one texture
  // this is also the step to add dynamic gpu-driven lighting
  BeginTextureMode(sceneBuf);
    BeginShaderMode(scenePrepShader);
      float time = GetTime();
      SetShaderValueTexture(scenePrepShader, GetShaderLocation(scenePrepShader, "uOcclusionMap"), occlusionMap.tex);
      SetShaderValueTexture(scenePrepShader, GetShaderLocation(scenePrepShader, "uEmissionMap"),  emissionMap.tex);
      SetShaderValue(scenePrepShader, GetShaderLocation(scenePrepShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);
      SetShaderValue(scenePrepShader, GetShaderLocation(scenePrepShader, "uTime"),       &time,       SHADER_UNIFORM_FLOAT);
      DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    EndShaderMode();
  EndTextureMode();

  // first render pass for JFA
  // create UV mask w/ prep shader
  BeginTextureMode(bufferA);
    BeginShaderMode(prepJfaShader);
      SetShaderValueTexture(prepJfaShader, GetShaderLocation(prepJfaShader, "uSceneMap"), sceneBuf.texture);
      SetShaderValue(prepJfaShader, GetShaderLocation(prepJfaShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);
      DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    EndShaderMode();
  EndTextureMode();

  // ping-pong buffering
  // alternate between two buffers so that we can implement a recursive shader
  // see https://mini.gmshaders.com/p/gm-shaders-mini-recursive-shaders-1308459
  for (int j = maxSteps; j >= 1; j /= 2) {
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

  // write distance field to another buffer
  // reduces strain as the cpu gets to send less data to the gpu for GI shader
  BeginTextureMode(distFieldBuf);
    BeginShaderMode(distFieldShader);
      SetShaderValueTexture(distFieldShader, GetShaderLocation(distFieldShader, "uJFA"), bufferA.texture);
      SetShaderValue(distFieldShader, GetShaderLocation(distFieldShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);
      DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    EndShaderMode();
  EndTextureMode();

  // display bufferA to main framebuffer
  BeginShaderMode(lightingShader);
    SetShaderValueTexture(lightingShader, GetShaderLocation(lightingShader, "uDistanceField"), distFieldBuf.texture);
    SetShaderValueTexture(lightingShader, GetShaderLocation(lightingShader, "uSceneMap"),      sceneBuf.texture);
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uRaysPerPx"),  &raysPerPx,  SHADER_UNIFORM_INT);
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uMaxSteps"),   &maxSteps,   SHADER_UNIFORM_INT);
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  EndShaderMode();

  UnloadRenderTexture(bufferB);
  UnloadRenderTexture(bufferC);
  UnloadRenderTexture(bufferA);
  UnloadRenderTexture(sceneBuf);
  UnloadRenderTexture(distFieldBuf);

  DrawTextureEx(user.brush.tex,
                Vector2{ (float)(GetMouseX() - user.brush.tex.width  / 2 * user.brushSize),
                         (float)(GetMouseY() - user.brush.tex.height / 2 * user.brushSize) },
                0.0,
                user.brushSize,
                Color{ 0, 0, 0, 128} );
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Demo::renderUI() {
  // dont draw our custom cursor if we are mousing over the UI
  ImGuiIO& io = ImGui::GetIO();
  if (!io.WantCaptureMouse) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_None);

    DrawTextureEx(cursor.tex,
                  Vector2{ (float)(GetMouseX() - cursor.img.width / 2 * CURSOR_SIZE),
                           (float)(GetMouseY() - cursor.img.height/ 2 * CURSOR_SIZE) },
                  0.0,
                  CURSOR_SIZE,
                  WHITE);
  }

  if (skipUIRendering) return;

  float h = 100;
  // if (user.mode == LIGHTING) h = 170;
  // if (help)  h += 115;
  if (debug) h += 178;

  ImGui::SetNextWindowSize(ImVec2{300, h});
  ImGui::SetNextWindowPos(ImVec2{4, 4});

  std::string str = "Drawing";
  if (user.mode == LIGHTING)     str = "Lighting";

  if (!ImGui::Begin("Mode", &debugWindowData.open, debugWindowData.flags)) {
    ImGui::End();
  } else {
    if (debug) {
      ImGui::SeparatorText("Debug");
      ImGui::Text("%d FPS", GetFPS());
      ImGui::SliderInt("##rays per px",   &raysPerPx, 0, 512, "rays per px = %i");
      ImGui::SliderInt("##max ray steps", &maxSteps, 0, 512, "max ray steps = %i");

      ImGui::Combo("canvas", &currentMap, "maze.png\0trees.png\0", 2);

      if (ImGui::SmallButton("set random colour"))  user.lightColor = RANDOM_COLOR;
      Vector4 col4 = ColorNormalize(user.lightColor);
      rlImGuiImageSizeV(&emissionMap.tex, {200, 200});
      float col[3] = { col4.x, col4.y, col4.z };
      ImGui::ColorEdit3("light color", col);
      user.lightColor = ColorFromNormalized(Vector4{col[0], col[1], col[2], 1.0});
    }
    ImGui::End();
  }
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// FIX: screenshots arent saved in a specified directory, might need to adapt raylib's screenshot function
void Demo::processKeyboardInput() {
  if (IsKeyPressed(KEY_ONE))   user.mode = DRAWING;
  if (IsKeyPressed(KEY_TWO))   user.mode = LIGHTING;

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
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Demo::processMouseInput() {
  // we do not want to be affecting the scene when we're clicking on the UI
  if (ImGui::GetIO().WantCaptureMouse) return;

  user.brushSize += GetMouseWheelMove() / 100;

  if      (user.brushSize < 0.05) user.brushSize = 0.05;
  else if (user.brushSize > 1.0) user.brushSize = 1.0;

  ImageTexture canvas = occlusionMap;
  if (user.mode == LIGHTING) canvas = emissionMap;

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
              (user.mode == LIGHTING) ? BLACK : WHITE); // emission map uses a black background, occlusion map uses a white background
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
    emissionMap.img = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
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
  emissionMap.img = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
  RELOAD_CANVAS();
}
