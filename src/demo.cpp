#include "demo.h"

#define CANVAS_MAP "res/textures/canvas/maze.png"
#define RELOAD_CANVAS() UnloadTexture(occlusionMap.tex); \
                        occlusionMap.tex = LoadTextureFromImage(occlusionMap.img); \
                        UnloadTexture(emissionMap.tex); \
                        emissionMap.tex = LoadTextureFromImage(emissionMap.img);

Demo::Demo() {
  // --- MISC PARAMETERS
  maxSteps = 64;
  jfaSteps = 128;
  raysPerPx = 64;
  pointA = 0.0;
  pointB = 1.0;
  orbs = 0.0;
  gi = false;

  user.mode= DRAWING;
  userSetRandomColor();

  // UI
  skipUIRendering = false;
  debugShowBuffers = false;
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

  HideCursor();

  // --- LOAD RESOURCES

  lastMousePos = {0, 0};

  cursorTex = LoadTexture("res/textures/cursor.png");

  user.brush.img = LoadImage("res/textures/brush.png");
  user.brush.tex = LoadTextureFromImage(user.brush.img);
  user.brushSize = 0.25;

  occlusionMap.img = LoadImage(CANVAS_MAP);
  occlusionMap.tex = LoadTextureFromImage(occlusionMap.img);
  emissionMap.img = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
  emissionMap.tex = LoadTextureFromImage(emissionMap.img);

  // automatically load fragment shaders in the `res/shaders` directory
  FilePathList shaderFiles = LoadDirectoryFilesEx("res/shaders", ".frag", false);
  for (int i = 0; i < shaderFiles.count; i++) {
    std::string str = shaderFiles.paths[i];
    str.erase(0, 12);
    if (str != "broken.frag") loadShader(str);
  }
  UnloadDirectoryFiles(shaderFiles);

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

  bufferA         = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
  bufferB         = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
  bufferC         = bufferA;
  radianceBufferA = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
  radianceBufferB = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
  radianceBufferC = radianceBufferA;
  sceneBuf        = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
  distFieldBuf    = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
  lastFrameBuf    = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);

  changeBitDepth(bufferA,      PIXELFORMAT_UNCOMPRESSED_R32G32B32A32);
  changeBitDepth(bufferB,      PIXELFORMAT_UNCOMPRESSED_R32G32B32A32);
  changeBitDepth(bufferC,      PIXELFORMAT_UNCOMPRESSED_R32G32B32A32);
  changeBitDepth(sceneBuf,     PIXELFORMAT_UNCOMPRESSED_R5G5B5A1);
  changeBitDepth(distFieldBuf, PIXELFORMAT_UNCOMPRESSED_R16);
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Demo::update() {
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Demo::render() {
  ClearBackground(PINK);

  // const Shader& rcShader  = shaders["rc.frag"];
  const Shader& rcShader        = shaders["rc.frag"];
  const Shader& giShader        = shaders["gi.frag"];
  const Shader& jfaShader       = shaders["jfa.frag"];
  const Shader& prepJfaShader   = shaders["prepjfa.frag"];
  const Shader& scenePrepShader = shaders["prepscene.frag"];
  const Shader& distFieldShader = shaders["distfield.frag"];
  const Shader& prepGiShader    = shaders["prepgi.frag"];
  const Shader& finalShader     = shaders["final.frag"];

  Vector2 resolution = { SCREEN_WIDTH, SCREEN_HEIGHT };

  // create scene texture - combining emission & occlusion maps into one texture
  // this is also the step to add dynamic gpu-driven lighting
  BeginTextureMode(sceneBuf);
    ClearBackground(BLANK);
    BeginShaderMode(scenePrepShader);
      float time = GetTime();
      Vector2 mousePos = GetMousePosition();
      SetShaderValueTexture(scenePrepShader, GetShaderLocation(scenePrepShader, "uOcclusionMap"),    occlusionMap.tex);
      SetShaderValueTexture(scenePrepShader, GetShaderLocation(scenePrepShader, "uEmissionMap"),     emissionMap.tex);
      SetShaderValue(scenePrepShader, GetShaderLocation(scenePrepShader, "uMousePos"),   &mousePos, SHADER_UNIFORM_VEC2);
      SetShaderValue(scenePrepShader, GetShaderLocation(scenePrepShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);
      SetShaderValue(scenePrepShader, GetShaderLocation(scenePrepShader, "uTime"),       &time,       SHADER_UNIFORM_FLOAT);
      SetShaderValue(scenePrepShader, GetShaderLocation(scenePrepShader, "uOrbs"),       &orbs,       SHADER_UNIFORM_INT);
      DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    EndShaderMode();
  EndTextureMode();

  // first render pass for JFA
  // create UV mask w/ prep shader
  BeginTextureMode(bufferA);
    ClearBackground(BLANK);
    BeginShaderMode(prepJfaShader);
      SetShaderValueTexture(prepJfaShader, GetShaderLocation(prepJfaShader, "uSceneMap"), sceneBuf.texture);
      SetShaderValue(prepJfaShader, GetShaderLocation(prepJfaShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);
      DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    EndShaderMode();
  EndTextureMode();

  // // ping-pong buffering
  // // alternate between two buffers so that we can implement a recursive shader
  // // see https://mini.gmshaders.com/p/gm-shaders-mini-recursive-shaders-1308459
  for (int j = jfaSteps*2; j >= 1; j /= 2) {
    bufferC = bufferA;
    bufferA = bufferB;
    bufferB = bufferC;

    BeginTextureMode(bufferA);
      ClearBackground(BLANK);
      BeginShaderMode(jfaShader);
        SetShaderValueTexture(jfaShader, GetShaderLocation(jfaShader, "uCanvas"), bufferB.texture);
        SetShaderValue(jfaShader, GetShaderLocation(jfaShader, "uJumpSize"), &j, SHADER_UNIFORM_INT);
        SetShaderValue(jfaShader, GetShaderLocation(jfaShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
      EndShaderMode();
    EndTextureMode();
  }

  // write distance field to another buffer
  // reduces strain as the cpu gets to send less data to the gpu for GI shader
  BeginTextureMode(distFieldBuf);
    ClearBackground(BLANK);
    BeginShaderMode(distFieldShader);
      SetShaderValueTexture(distFieldShader, GetShaderLocation(distFieldShader, "uJFA"), bufferA.texture);
      SetShaderValue(distFieldShader, GetShaderLocation(distFieldShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);
      DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    EndShaderMode();
  EndTextureMode();

  resolution *= GetWindowScaleDPI();

  BeginTextureMode(radianceBufferB);
    BeginShaderMode(prepGiShader);
      SetShaderValueTexture(prepGiShader, GetShaderLocation(prepGiShader, "uSceneMap"), sceneBuf.texture);
      SetShaderValueTexture(prepGiShader, GetShaderLocation(prepGiShader, "uLastFrame"), lastFrameBuf.texture);
      SetShaderValue(prepGiShader, GetShaderLocation(prepGiShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);
      DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    EndShaderMode();
  EndTextureMode();

  if (gi) {
    BeginTextureMode(radianceBufferA);
      BeginShaderMode(giShader);
        ClearBackground(BLANK);
        SetShaderValueTexture(giShader, GetShaderLocation(giShader, "uDistanceField"), distFieldBuf.texture);
        SetShaderValueTexture(giShader, GetShaderLocation(giShader, "uSceneMap"),      sceneBuf.texture);
        SetShaderValueTexture(giShader, GetShaderLocation(giShader, "uLastFrame"),     radianceBufferB.texture);
        SetShaderValue(giShader, GetShaderLocation(giShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);
        SetShaderValue(giShader, GetShaderLocation(giShader, "uRaysPerPx"),  &raysPerPx,  SHADER_UNIFORM_INT);
        SetShaderValue(giShader, GetShaderLocation(giShader, "uMaxSteps"),   &maxSteps,   SHADER_UNIFORM_INT);
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
      EndShaderMode();
    EndTextureMode();
  } else {
    for (int i = 0; i < 2; i++) {
      radianceBufferC = radianceBufferA;
      radianceBufferA = radianceBufferB;
      radianceBufferB = radianceBufferC;


      #define BASE_RAY_COUNT 16
      float intervalStart = i == 0 ? 0.0   : 0.125;
      float intervalEnd   = i == 0 ? 0.125 : 1.0;
      int rayCount = pow(BASE_RAY_COUNT, i+1); // angular resolution
      float linearResolution = i == 0 ? 1.0 : 4.0;

      BeginTextureMode(radianceBufferA);
        BeginShaderMode(rcShader);
          ClearBackground(BLANK);
          SetShaderValueTexture(rcShader, GetShaderLocation(rcShader, "uDistanceField"), distFieldBuf.texture);
          SetShaderValueTexture(rcShader, GetShaderLocation(rcShader, "uSceneMap"),      sceneBuf.texture);
          SetShaderValueTexture(rcShader, GetShaderLocation(rcShader, "uLastPass"),      radianceBufferC.texture);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uResolution"), &resolution,   SHADER_UNIFORM_VEC2);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uRaysPerPx"),  &rayCount, SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uLinearResolution"),  &linearResolution, SHADER_UNIFORM_FLOAT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uMaxSteps"),   &maxSteps,     SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uPointA"),   &intervalStart,  SHADER_UNIFORM_FLOAT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uPointB"),   &intervalEnd,    SHADER_UNIFORM_FLOAT);
          DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
        EndShaderMode();
      EndTextureMode();
    }
  }

  BeginTextureMode(lastFrameBuf);
    DrawTextureRec(radianceBufferA.texture, {0, 0.0, SCREEN_WIDTH, SCREEN_HEIGHT}, {0.0, 0.0}, WHITE);
  EndTextureMode();

  BeginShaderMode(finalShader);
    SetShaderValueTexture(finalShader, GetShaderLocation(finalShader, "uCanvas"), lastFrameBuf.texture);
    SetShaderValue(finalShader, GetShaderLocation(finalShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  EndShaderMode();

  if (!skipUIRendering) {
    DrawTextureEx(user.brush.tex,
                  Vector2{ (float)(GetMouseX() - user.brush.tex.width  / 2 * user.brushSize),
                           (float)(GetMouseY() - user.brush.tex.height / 2 * user.brushSize) },
                  0.0,
                  user.brushSize,
                  Color{ 0, 0, 0, 128} );
  }
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Demo::renderUI() {
  // dont draw our custom cursor if we are mousing over the UI
  ImGuiIO& io = ImGui::GetIO();
  if (!io.WantCaptureMouse && !skipUIRendering) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_None);
    DrawTextureEx(cursorTex,
                  Vector2{ (float)(GetMouseX() - cursorTex.width / 2 * CURSOR_SIZE),
                           (float)(GetMouseY() - cursorTex.height/ 2 * CURSOR_SIZE) },
                  0.0,
                  CURSOR_SIZE,
                  WHITE);
  }

  if (skipUIRendering) return;

  float h = 100;
  if (debug) {
    h += 180;
    if (debugShowBuffers) h += 60*4;
  }
  ImGui::SetNextWindowSize(ImVec2{250, h});
  ImGui::SetNextWindowPos(ImVec2{4, 4});

  if (!ImGui::Begin("Mode", &debugWindowData.open, debugWindowData.flags)) {
    ImGui::End();
  } else {
    if (ImGui::SmallButton("set r(a)ndom colour")) userSetRandomColor();
    Vector4 col4 = ColorNormalize(user.lightColor);
    float col[3] = { col4.x, col4.y, col4.z };
    ImGui::ColorEdit3("light color", col);
    user.lightColor = ColorFromNormalized(Vector4{col[0], col[1], col[2], 1.0});
    if (ImGui::SmallButton("(c)lear canvas")) clear();
    ImGui::SameLine();
    if (ImGui::SmallButton("(r)eload canvas")) reload();
    if (debug) {
      ImGui::SeparatorText("Debug");
      ImGui::Text("%d FPS", GetFPS());
      ImGui::SliderInt("##rays per px",   &raysPerPx, 0, 512, "rays per px = %i");
      ImGui::SliderInt("##max ray steps", &maxSteps, 0, 512, "max ray steps = %i");
      ImGui::SliderInt("##jfa steps",     &jfaSteps, 0, 512, "jfa steps = %i");
      ImGui::SliderFloat("##point a",     &pointA, 0.0, 1.0, "a = %f");
      ImGui::SliderFloat("##point b",     &pointB, 0.0, 1.0, "b = %f");
      bool orbsBool = (orbs == 1);
      ImGui::Checkbox("light orb circle", &orbsBool);
      orbs = (int)orbsBool;
      ImGui::Checkbox("gi", &gi);
      if (ImGui::SmallButton("show buffers")) debugShowBuffers = !debugShowBuffers;
      if (debugShowBuffers) {
        if (ImGui::BeginTable("buffer_table", 2)) {
          // ImGui::TableNextRow();
          //   ImGui::TableSetColumnIndex(0);
          //   rlImGuiImageSizeV(&emissionMap.tex,      {80, 60});
          //   ImGui::TableSetColumnIndex(1);
          //   rlImGuiImageSizeV(&occlusionMap.tex,     {80, 60});
          ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            rlImGuiImageSizeV(&sceneBuf.texture,     {80, 60});
          ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            rlImGuiImageSizeV(&bufferA.texture,      {80, 60});
            ImGui::TableSetColumnIndex(1);
            rlImGuiImageSizeV(&distFieldBuf.texture, {80, 60});
          ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            rlImGuiImageSizeV(&radianceBufferA.texture,      {80, 60});
            ImGui::TableSetColumnIndex(1);
            rlImGuiImageSizeV(&radianceBufferB.texture,      {80, 60});
          ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            rlImGuiImageSizeV(&radianceBufferC.texture,      {80, 60});
          ImGui::EndTable();
        }
      }
      ImGui::SeparatorText("Build Info");
      std::string str = "built ";
      str += __DATE__;
      str += " at ";
      str += __TIME__;
      ImGui::Text(str.c_str());
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
  // if (IsKeyPressed(KEY_F2)) {
  //   printf("Taking screenshot.\n");
  //   if (!DirectoryExists("screenshots")) MakeDirectory("screenshots");
  //   std::string pwd = GetWorkingDirectory();
  //   ChangeDirectory("screenshots");
  //   TakeScreenshot("screenshot.png");
  //   ChangeDirectory(pwd.c_str());
  // }
  // if (IsKeyPressed(KEY_F12)) ToggleFullscreen();

  if (IsKeyPressed(KEY_C)) clear();
  if (IsKeyPressed(KEY_R)) reload();
  if (IsKeyPressed(KEY_A)) userSetRandomColor();
  if (IsKeyPressed(KEY_H)) help = !help;
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Demo::processMouseInput() {
  // we do not want to be affecting the scene when we're clicking on the UI
  if (ImGui::GetIO().WantCaptureMouse) return;

  user.brushSize += GetMouseWheelMove() / 100;
  if      (user.brushSize < 0.05) user.brushSize = 0.05;
  else if (user.brushSize > 1.0)  user.brushSize = 1.0;

  if (IsMouseButtonDown(0) || IsMouseButtonDown(1)) {
    // TODO: CPU-based drawing is not very efficient
    auto draw = [this](ImageTexture canvas, Color color, Vector2 pos = GetMousePosition()) {
      ImageDraw(&canvas.img,
                this->user.brush.img,
                Rectangle{ 0, 0, (float)canvas.img.width, (float)canvas.img.height },
                Rectangle{ static_cast<float>(pos.x - this->user.brush.img.width  / 2 * this->user.brushSize),
                           static_cast<float>(pos.y - this->user.brush.img.height / 2 * this->user.brushSize),
                           static_cast<float>(this->user.brush.img.width  * this->user.brushSize),
                           static_cast<float>(this->user.brush.img.height * this->user.brushSize) },
                color);
    };

    ImageTexture& canvas = (user.mode == LIGHTING) ? emissionMap : occlusionMap;
    Color color;
    if (IsMouseButtonDown(0) && !IsKeyDown(KEY_LEFT_CONTROL)) {                                  // DRAW
      color = (user.mode == LIGHTING) ? user.lightColor : BLACK;
    } else if (IsMouseButtonDown(1) || (IsKeyDown(KEY_LEFT_CONTROL) && IsMouseButtonDown(0))) { // ERASE
      color = (user.mode == LIGHTING) ? BLACK : WHITE;
    }

    // Lerp between current mouse position and previous recorded mouse position so that we can have nice smooth lines when drawing
    // this appears to be a very expensive operation! hence why it is enclosed in this if statement
    // -20 FPS when drawing! crazy
    for (int i = 0; i < 8; i++) {
      Vector2 pos = GetSplinePointLinear(lastMousePos, GetMousePosition(), i/10.0);
      draw(canvas, color, pos);
    }
    draw(canvas, color);
    RELOAD_CANVAS();
  }

  lastMousePos = GetMousePosition();
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Demo::userSetRandomColor() {
  Color col = ColorFromNormalized(Vector4{(std::sin(static_cast<float>(GetTime()))   + 1) / 2,
                                          (std::cos(static_cast<float>(GetTime()))   + 1) / 2,
                                          (std::sin(static_cast<float>(GetTime())*2) + 1) / 2,
                                  1.0 });

  // at least one colour channel needs to be at maximum
  if (col.r < 255 && col.g < 255 & col.b < 255) {
    switch (GetRandomValue(0, 2)) {
      case 0:
        col.r = 255;
        break;
      case 1:
        col.g = 255;
        break;
      case 2:
        col.b = 255;
        break;
    }
  }

  user.lightColor = col;
}

void Demo::loadShader(std::string shader) {
  std::string path = "res/shaders/" + shader;
  Shader s = LoadShader(0, path.c_str());
  if (!IsShaderValid(s)) {
    std::cout << "ERR: '" << shader << "' is broken." << std::endl;
    UnloadShader(s);
    s = LoadShader("res/shaders/default.vert", "res/shaders/broken.frag");
  } else {
    std::cout << "Loaded '" << shader << "' successfully." << std::endl;
  }
  shaders[shader] = s;
}

// reload based on what mode we're in, unless we're press control
// if we're pressing control we reload shaders
//
// DRAWING: load canvas image texture
// LIGHTING: set lighting to how it is when the programme starts
void Demo::reload() {
  if (IsKeyDown(KEY_LEFT_CONTROL)) {
    std::cout << "Reloading shaders." << std::endl;
    for (auto const& [key, val] : shaders) {
      loadShader(key);
    }
  } else {
    printf("Replacing canvas.\n");
    occlusionMap.img = LoadImage(CANVAS_MAP);
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
