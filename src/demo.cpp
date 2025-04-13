#include "demo.h"

Demo::Demo() {
  // --- MISC PARAMETERS
  maxRaySteps = 64;
  jfaSteps = 128;

  orbs = false;
  srgb = false;

  gi = false;
  giRayCount = 64;
  giNoise = true;
  giMixFactor = 0.7;
  giDecayRate = 1.3;

  rcRayCount = 4;
  cascadeAmount = 5;
  cascadeDisplayIndex = 0;
  rcBilinear = true;
  rcDisableMerging = false;
  baseInterval = 0.5;

  user.mode = DRAWING;
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

  ImGui::GetIO().IniFilename = NULL;

  HideCursor();

  // --- LOAD RESOURCES

  lastMousePos = {0, 0};

  cursorTex = LoadTexture("res/textures/cursor.png");

  user.brushTexture = LoadTexture("res/textures/brush.png");
  user.brushSize = 0.25;

  // automatically load fragment shaders in the `res/shaders` directory
  FilePathList shaderFiles = LoadDirectoryFilesEx("res/shaders", ".frag", false);
  for (int i = 0; i < shaderFiles.count; i++) {
    std::string str = shaderFiles.paths[i];
    str.erase(0, 12);
    if (str != "broken.frag") loadShader(str);
  }
  UnloadDirectoryFiles(shaderFiles);

  setBuffers();
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Demo::update() {
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Demo::render() {
  ClearBackground(BLACK);

  // -------------------------------- shader variables

  const Shader& rcShader        = shaders["rc.frag"];
  const Shader& giShader        = shaders["gi.frag"];
  const Shader& jfaShader       = shaders["jfa.frag"];
  const Shader& prepJfaShader   = shaders["prepjfa.frag"];
  const Shader& scenePrepShader = shaders["prepscene.frag"];
  const Shader& distFieldShader = shaders["distfield.frag"];
  const Shader& finalShader     = shaders["final.frag"];
  const Shader& drawShader      = shaders["draw.frag"];

  // uniforms
  Vector2 resolution = { (float)GetScreenWidth(), (float)GetScreenHeight() };
  float   time = GetTime();
  Vector2 mousePos = GetMousePosition();
  int     mouseDown = (IsMouseButtonDown(0) || IsMouseButtonDown(1)) && !ImGui::GetIO().WantCaptureMouse;
  int rainbowAnimationInt = rainbowAnimation;
  int drawRainbowInt = drawRainbow;
  Vector4 color;

  if (user.mode == DRAWING)
    color = (IsMouseButtonDown(1)) ? Vector4{1.0, 1.0, 1.0, 1.0} : Vector4{0.0, 0.0, 0.0, 1.0};
  else // user.mode == LIGHTING
    color = (IsMouseButtonDown(1)) ? Vector4{0.0, 0.0, 0.0, 1.0} : ColorNormalize(user.brushColor);

  // color = Vector4{1.0, 1.0, 1.0, 1.0};

  // -------------------------------- scene mapping

  // drawing to emission or occlusion map depending on `user.mode`
  BeginTextureMode((user.mode == DRAWING) ? occlusionBuf : emissionBuf);
    BeginShaderMode(drawShader);
      SetShaderValue(drawShader, GetShaderLocation(drawShader, "uTime"),         &time,           SHADER_UNIFORM_FLOAT);
      SetShaderValue(drawShader, GetShaderLocation(drawShader, "uMousePos"),     &mousePos,       SHADER_UNIFORM_VEC2);
      SetShaderValue(drawShader, GetShaderLocation(drawShader, "uLastMousePos"), &lastMousePos,   SHADER_UNIFORM_VEC2);
      SetShaderValue(drawShader, GetShaderLocation(drawShader, "uBrushSize"),    &user.brushSize, SHADER_UNIFORM_FLOAT);
      SetShaderValue(drawShader, GetShaderLocation(drawShader, "uBrushColor"),   &color,          SHADER_UNIFORM_VEC4);
      SetShaderValue(drawShader, GetShaderLocation(drawShader, "uResolution"),   &resolution,     SHADER_UNIFORM_VEC2);
      SetShaderValue(drawShader, GetShaderLocation(drawShader, "uMouseDown"),    &mouseDown,      SHADER_UNIFORM_INT);
      SetShaderValue(drawShader, GetShaderLocation(drawShader, "uRainbow"),      &drawRainbowInt, SHADER_UNIFORM_INT);
      DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
    EndShaderMode();
  EndTextureMode();

  // combine occlusion & emission map to be used in JFA and lighting passes
  // this shader is also an opportunity to add SDFs (such as the orbs)
  BeginTextureMode(sceneBuf);
    ClearBackground(BLANK);
    BeginShaderMode(scenePrepShader);
      int orbsInt = orbs;
      SetShaderValueTexture(scenePrepShader, GetShaderLocation(scenePrepShader, "uOcclusionMap"),    occlusionBuf.texture);
      SetShaderValueTexture(scenePrepShader, GetShaderLocation(scenePrepShader, "uEmissionMap"),     emissionBuf.texture);
      SetShaderValue(scenePrepShader, GetShaderLocation(scenePrepShader, "uMousePos"),   &mousePos,            SHADER_UNIFORM_VEC2);
      SetShaderValue(scenePrepShader, GetShaderLocation(scenePrepShader, "uResolution"), &resolution,          SHADER_UNIFORM_VEC2);
      SetShaderValue(scenePrepShader, GetShaderLocation(scenePrepShader, "uTime"),       &time,                SHADER_UNIFORM_FLOAT);
      SetShaderValue(scenePrepShader, GetShaderLocation(scenePrepShader, "uOrbs"),       &orbsInt,             SHADER_UNIFORM_INT);
      SetShaderValue(scenePrepShader, GetShaderLocation(scenePrepShader, "uRainbow"),    &rainbowAnimationInt, SHADER_UNIFORM_INT);
      DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
    EndShaderMode();
  EndTextureMode();

  // -------------------------------- jump flooding algorithm / distance field generation

  // first render pass for JFA
  // create UV mask w/ prep shader
  BeginTextureMode(bufferA);
    ClearBackground(BLANK);
    BeginShaderMode(prepJfaShader);
      SetShaderValueTexture(prepJfaShader, GetShaderLocation(prepJfaShader, "uSceneMap"), sceneBuf.texture);
      SetShaderValue(prepJfaShader, GetShaderLocation(prepJfaShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);
      DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
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
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
      EndShaderMode();
    EndTextureMode();
  }

  // write distance field to another buffer
  // reduces strain as the cpu gets to send less data to the gpu for lighting shaders
  BeginTextureMode(distFieldBuf);
    ClearBackground(BLANK);
    BeginShaderMode(distFieldShader);
      SetShaderValueTexture(distFieldShader, GetShaderLocation(distFieldShader, "uJFA"), bufferA.texture);
      SetShaderValue(distFieldShader, GetShaderLocation(distFieldShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);
      DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
    EndShaderMode();
  EndTextureMode();

  // -------------------------------- lighting pass

  int srgbInt = srgb;

  // --------------- traditional GI

  if (gi) {
    int giNoiseInt = giNoise;
    BeginTextureMode(radianceBufferA);
      BeginShaderMode(giShader);
        ClearBackground(BLANK);
        SetShaderValueTexture(giShader, GetShaderLocation(giShader, "uDistanceField"), distFieldBuf.texture);
        SetShaderValueTexture(giShader, GetShaderLocation(giShader, "uSceneMap"),      sceneBuf.texture);
        SetShaderValueTexture(giShader, GetShaderLocation(giShader, "uLastFrame"),     lastFrameBuf.texture);
        SetShaderValue(giShader, GetShaderLocation(giShader, "uResolution"), &resolution,  SHADER_UNIFORM_VEC2);
        SetShaderValue(giShader, GetShaderLocation(giShader, "uRaysPerPx"),  &giRayCount,  SHADER_UNIFORM_INT);
        SetShaderValue(giShader, GetShaderLocation(giShader, "uMaxSteps"),   &maxRaySteps, SHADER_UNIFORM_INT);
        SetShaderValue(giShader, GetShaderLocation(giShader, "uSrgb"),       &srgbInt,     SHADER_UNIFORM_INT);
        SetShaderValue(giShader, GetShaderLocation(giShader, "uNoise"),      &giNoiseInt,  SHADER_UNIFORM_INT);
        SetShaderValue(giShader, GetShaderLocation(giShader, "uDecayRate"),  &giDecayRate, SHADER_UNIFORM_FLOAT);
        SetShaderValue(giShader, GetShaderLocation(giShader, "uMixFactor"),  &giMixFactor, SHADER_UNIFORM_FLOAT);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
      EndShaderMode();
    EndTextureMode();
  } else {

  // --------------- radiance cascades

    int rcDisableMergingInt = rcDisableMerging;
    for (int i = cascadeAmount; i >= 0; i--) {
      radianceBufferC = radianceBufferA;
      radianceBufferA = radianceBufferB;
      radianceBufferB = radianceBufferC;

      if (rcBilinear)
        SetTextureFilter(radianceBufferC.texture, TEXTURE_FILTER_BILINEAR);
      else
        SetTextureFilter(radianceBufferC.texture, TEXTURE_FILTER_POINT);

      BeginTextureMode(radianceBufferA);
        BeginShaderMode(rcShader);
          ClearBackground(BLANK);
          SetShaderValueTexture(rcShader, GetShaderLocation(rcShader, "uDistanceField"), distFieldBuf.texture);
          SetShaderValueTexture(rcShader, GetShaderLocation(rcShader, "uSceneMap"),      sceneBuf.texture);
          SetShaderValueTexture(rcShader, GetShaderLocation(rcShader, "uLastPass"),      radianceBufferC.texture);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uResolution"),          &resolution,          SHADER_UNIFORM_VEC2);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uBaseRayCount"),        &rcRayCount,          SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uMaxSteps"),            &maxRaySteps,         SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uBaseInterval"),        &baseInterval,        SHADER_UNIFORM_FLOAT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uDisableMerging"),      &rcDisableMergingInt, SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uCascadeDisplayIndex"), &cascadeDisplayIndex, SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uCascadeIndex"),        &i,                   SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uCascadeAmount"),       &cascadeAmount,       SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uSrgb"),                &srgbInt,             SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uDecayRate"),           &giDecayRate,         SHADER_UNIFORM_FLOAT);
          DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
        EndShaderMode();
      EndTextureMode();
    }
  }

  resolution *= GetWindowScaleDPI();

  // -------------------------------- save to lastFrameBuf for next frame (for traditional GI) & display to main framebuffer

  BeginTextureMode(lastFrameBuf);
    DrawTextureRec(radianceBufferA.texture, {0, 0.0, (float)GetScreenWidth(), (float)GetScreenHeight()}, {0.0, 0.0}, WHITE);
  EndTextureMode();

  BeginShaderMode(finalShader);
    SetShaderValueTexture(finalShader, GetShaderLocation(finalShader, "uCanvas"), lastFrameBuf.texture); //(srgb) ? occlusionBuf.texture : emissionBuf.texture);
    SetShaderValue(finalShader, GetShaderLocation(finalShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
  EndShaderMode();

  // if (!skipUIRendering) {
    DrawTextureEx(user.brushTexture,
                  Vector2{ (float)(GetMouseX() - user.brushTexture.width  / 2 * user.brushSize),
                           (float)(GetMouseY() - user.brushTexture.height / 2 * user.brushSize) },
                  0.0,
                  user.brushSize,
                  Color{ 0, 0, 0, 128} );
  // }
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Demo::renderUI() {
  // dont draw our custom cursor if we are mousing over the UI
  ImGuiIO& io = ImGui::GetIO();
  if (!io.WantCaptureMouse) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_None);
    DrawTextureEx(cursorTex,
                  Vector2{ (float)(GetMouseX() - cursorTex.width / 2 * CURSOR_SIZE),
                           (float)(GetMouseY() - cursorTex.height/ 2 * CURSOR_SIZE) },
                  0.0,
                  CURSOR_SIZE,
                  WHITE);
  }

  if (skipUIRendering) return;

  float h = 400;
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
    Vector4 col4 = ColorNormalize(user.brushColor);
    float col[3] = { col4.x, col4.y, col4.z };
    ImGui::ColorEdit3("light color", col);
    user.brushColor = ColorFromNormalized(Vector4{col[0], col[1], col[2], 1.0});

    ImGui::SeparatorText("Scenes");

    if (ImGui::SmallButton("(c)lear")) setScene(CLEAR);
    ImGui::SameLine();
    if (ImGui::SmallButton("maze")) setScene(MAZE);
    ImGui::SameLine();
    if (ImGui::SmallButton("trees")) setScene(TREES);

    if (ImGui::SmallButton("penumbra")) setScene(PENUMBRA);
    ImGui::SameLine();
    if (ImGui::SmallButton("penumbra2")) setScene(PENUMBRA2);

    ImGui::SeparatorText("Lighting");

    ImGui::Checkbox("trad. gi", &gi);
    ImGui::SameLine();
    bool tmp = !gi;
    ImGui::Checkbox("radiance cascades", &tmp);
    gi = !tmp;

    ImGui::Checkbox("light orb circle", &orbs);
    ImGui::Checkbox("sRGB conversion", &srgb);
    ImGui::Checkbox("draw rainbow", &drawRainbow);
    ImGui::Checkbox("rainbow animation", &rainbowAnimation);
    ImGui::SliderFloat("##mix factor",   &giMixFactor, 0.0, 1.0, "mix factor = %f");
    ImGui::SliderFloat("##decay rate",   &giDecayRate, 0.0, 2.0, "decay rate = %f");

    if (gi) {
      ImGui::SeparatorText("traditional gi");
      ImGui::SliderInt("##rays per px",   &giRayCount, 0, 512, "ray count = %i");
      ImGui::Checkbox("noise", &giNoise);
    } else {
      ImGui::SeparatorText("radiance cascades");

      auto setParams = [this](int n){
        rcRayCount = pow(4, n);
      };

      ImGui::Text("branching factor");
      if (ImGui::SmallButton("1"))
        setParams(1);
      ImGui::SameLine();
      if (ImGui::SmallButton("2"))
        setParams(2);
      ImGui::SameLine();
      if (ImGui::SmallButton("3"))
        setParams(3);
      ImGui::SliderInt("##rays per px",   &rcRayCount, 0, 64,  "base ray count = %i");
      // ImGui::SliderInt("##rays per px",   &rcRayCount, 0, 64,  "top cascade probe amount = %i");
      ImGui::SliderInt("##cascade amount",   &cascadeAmount, 0, 32, "cascade amount = %i");
      ImGui::SliderInt("##cascade display",   &cascadeDisplayIndex, 0, cascadeAmount-1, "display cascade = %i");
      ImGui::SliderFloat("##base interval",   &baseInterval, 0, 64.0, "base interval = %fpx");
      ImGui::Checkbox("bilinear interpolation", &rcBilinear);
      ImGui::Checkbox("disable merging", &rcDisableMerging);
    }

    if (debug) {
      ImGui::SeparatorText("Debug");
      ImGui::Text("%d FPS", GetFPS());

      if (ImGui::SmallButton("show buffers")) debugShowBuffers = !debugShowBuffers;
      if (debugShowBuffers) {
        if (ImGui::BeginTable("buffer_table", 2)) {
          ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            rlImGuiImageSizeV(&occlusionBuf.texture,      {80, 60});
            ImGui::TableSetColumnIndex(1);
            rlImGuiImageSizeV(&emissionBuf.texture,     {80, 60});
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

  if (IsKeyPressed(KEY_C)) setScene(CLEAR);
  if (IsKeyPressed(KEY_R)) {
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
      std::cout << "Reloading shaders." << std::endl;
      for (auto const& [key, val] : shaders) {
        loadShader(key);
      }
    } else {
      setScene(CLEAR);
    }
  }
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

  lastMousePos = GetMousePosition();
}

void Demo::resize() {
  setBuffers();
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#define DRAW_TEXTURE_STRETCH(file) DrawTexturePro(LoadTexture(file), Rectangle{0, 0, 800, -600}, Rectangle{0, 0, GetScreenWidth(), GetScreenHeight()}, Vector2{0, 0}, 0.0, WHITE);

void Demo::setBuffers() {
  Texture2D emissionTexture = emissionBuf.texture;
  Texture2D occlusionTexture = occlusionBuf.texture;

  UnloadRenderTexture(bufferA);
  UnloadRenderTexture(bufferB);
  UnloadRenderTexture(bufferC);
  UnloadRenderTexture(radianceBufferA);
  UnloadRenderTexture(radianceBufferB);
  UnloadRenderTexture(radianceBufferC);
  UnloadRenderTexture(sceneBuf);
  UnloadRenderTexture(distFieldBuf);
  UnloadRenderTexture(lastFrameBuf);
  UnloadRenderTexture(occlusionBuf);
  UnloadRenderTexture(emissionBuf);

  // change bit depth for bufferA, B, & C so that we can encode texture coordinates without losing data
  // default Raylib FBOs have a bit depth of 8 per channel, which would only cover for a window of maximum size 255x255
  // we can also save some memory by reducing bit depth of buffers to what is strictly required
  auto changeBitDepth = [](RenderTexture2D &buffer, PixelFormat pixformat) {
    rlEnableFramebuffer(buffer.id);
      rlUnloadTexture(buffer.texture.id);
      buffer.texture.id = rlLoadTexture(NULL, GetScreenWidth(), GetScreenHeight(), pixformat, 1);
      buffer.texture.width = GetScreenWidth();
      buffer.texture.height = GetScreenHeight();
      buffer.texture.format = pixformat;
      buffer.texture.mipmaps = 1;
      rlFramebufferAttach(buffer.id, buffer.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
    rlDisableFramebuffer();
  };

  bufferA         = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
  bufferB         = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
  bufferC         = bufferA;
  radianceBufferA = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
  radianceBufferB = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
  radianceBufferC = radianceBufferA;
  sceneBuf        = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
  distFieldBuf    = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
  lastFrameBuf    = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
  emissionBuf     = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
  occlusionBuf    = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

  changeBitDepth(bufferA,      PIXELFORMAT_UNCOMPRESSED_R32G32B32A32);
  changeBitDepth(bufferB,      PIXELFORMAT_UNCOMPRESSED_R32G32B32A32);
  changeBitDepth(bufferC,      PIXELFORMAT_UNCOMPRESSED_R32G32B32A32);
  changeBitDepth(sceneBuf,     PIXELFORMAT_UNCOMPRESSED_R5G5B5A1);
  changeBitDepth(distFieldBuf, PIXELFORMAT_UNCOMPRESSED_R16);
  changeBitDepth(occlusionBuf, PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA);
  changeBitDepth(emissionBuf,  PIXELFORMAT_UNCOMPRESSED_R5G5B5A1);

  BeginTextureMode(occlusionBuf);
    ClearBackground(WHITE);
    // DrawTexture(occlusionTexture, 0, 0, WHITE);
    DrawTexturePro(occlusionTexture, Rectangle{0, 0, occlusionTexture.width, occlusionTexture.height}, Rectangle{0, 0, GetScreenWidth(), GetScreenHeight()}, Vector2{0, 0}, 0.0, WHITE);
  EndTextureMode();

  BeginTextureMode(emissionBuf);
    ClearBackground(BLACK);
    DrawTexture(emissionTexture, 0, 0, WHITE);
  EndTextureMode();
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Demo::userSetRandomColor() {
  user.brushColor = ColorFromNormalized(Vector4{(std::sin(static_cast<float>(GetTime()))   + 1) / 2,
                                          (std::cos(static_cast<float>(GetTime()))   + 1) / 2,
                                          (std::sin(static_cast<float>(GetTime())*2) + 1) / 2,
                                  1.0 });
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

void Demo::setScene(Scene scene) {
  switch (scene) {
    case CLEAR:
      if (user.mode == DRAWING) {
        BeginTextureMode(occlusionBuf);
          ClearBackground(WHITE);
        EndTextureMode();
      } else {
        BeginTextureMode(emissionBuf);
          ClearBackground(BLACK);
        EndTextureMode();
      }
      break;
    case MAZE:
      BeginTextureMode(occlusionBuf);
        DRAW_TEXTURE_STRETCH("res/textures/canvas/maze.png")
      EndTextureMode();
      break;
    case TREES:
      BeginTextureMode(occlusionBuf);
        DRAW_TEXTURE_STRETCH("res/textures/canvas/trees.png")
      EndTextureMode();
      break;
    case PENUMBRA:
      BeginTextureMode(occlusionBuf);
        DRAW_TEXTURE_STRETCH("res/textures/canvas/penumbra.png")
      EndTextureMode();
      BeginTextureMode(emissionBuf);
        DRAW_TEXTURE_STRETCH("res/textures/canvas/penumbra_e.png")
      EndTextureMode();
      break;
    case PENUMBRA2:
      BeginTextureMode(occlusionBuf);
        DRAW_TEXTURE_STRETCH("res/textures/canvas/penumbra2.png")
      EndTextureMode();
      BeginTextureMode(emissionBuf);
        DRAW_TEXTURE_STRETCH("res/textures/canvas/penumbra2_e.png")
      EndTextureMode();
      break;
  }
}
