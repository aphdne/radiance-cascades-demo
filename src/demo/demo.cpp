#include "demo.h"

Demo::Demo() {
  setBuffers();

  screenshotWindowData.flags |= ImGuiWindowFlags_NoResize;
  screenshotWindowData.flags |= ImGuiWindowFlags_NoNav;
  screenshotWindowData.flags |= ImGuiWindowFlags_NoInputs;
  screenshotWindowData.flags |= ImGuiWindowFlags_NoTitleBar;
  screenshotWindowData.open = false;

  // automatically load fragment shaders in the `res/shaders` directory
  FilePathList shaderFiles = LoadDirectoryFilesEx("res/shaders", ".frag", false);
  for (int i = 0; i < shaderFiles.count; i++) {
    std::string str = shaderFiles.paths[i];
    str.erase(0, 12);
    if (str != "broken.frag") loadShader(str);
  }
  UnloadDirectoryFiles(shaderFiles);
}

void Demo::render() {
  prepScene();
  renderScene();
}

void Demo::renderUI() {
  if (!ImGui::GetIO().WantCaptureMouse)
    ImGui::SetMouseCursor(ImGuiMouseCursor_None);

  if (screenshotWindowData.open) {
    #define WIDTH 135
    #define HEIGHT 25

    ImGui::SetNextWindowBgAlpha(WINDOW_OPACITY);
    ImGui::SetNextWindowPos({(float)GetScreenWidth()/2 - WIDTH/2, (float)GetScreenHeight()/2 - HEIGHT/2});
    ImGui::SetNextWindowSize({WIDTH, HEIGHT});
    if (ImGui::Begin("Screenshot", &screenshotWindowData.open, screenshotWindowData.flags)) {
      ImGui::Text("Screenshot taken!");
      ImGui::End();
    }

    timeSinceScreenshot += GetFrameTime();
    if (timeSinceScreenshot > SCREENSHOT_POPUP_TIME)
      screenshotWindowData.open = false;
  }

  if (skipUIRendering) return;
}

void Demo::processMouseInput() {
  if (ImGui::GetIO().WantCaptureMouse) return;
}

void Demo::processKeyboardInput() {
  if (ImGui::GetIO().WantCaptureMouse) return;

  if (IsKeyPressed(KEY_GRAVE)) debug = !debug;
  if (IsKeyPressed(KEY_F1))    skipUIRendering = !skipUIRendering;
  if (IsKeyPressed(KEY_F2))    saveCanvas();

  if (IsKeyDown(KEY_R) && IsKeyDown(KEY_LEFT_CONTROL)) { // reload shaders
      std::cout << "Reloading shaders." << std::endl;
      for (auto const& [key, val] : shaders)
        loadShader(key);
  }
}

void Demo::renderScene() {
  Vector2 resolution = { (float)GetScreenWidth(), (float)GetScreenHeight() };

  const Shader& rcShader        = shaders["rc.frag"];
  const Shader& giShader        = shaders["gi.frag"];
  const Shader& jfaShader       = shaders["jfa.frag"];
  const Shader& prepJfaShader   = shaders["prepjfa.frag"];
  const Shader& distFieldShader = shaders["distfield.frag"];
  const Shader& finalShader     = shaders["final.frag"];

  // -------------------------------- jump flooding algorithm / distance field generation

  // first render pass for JFA
  // create UV mask w/ prep shader
  BeginTextureMode(jfaBufferA);
    ClearBackground(BLANK);
    BeginShaderMode(prepJfaShader);
      SetShaderValueTexture(prepJfaShader, GetShaderLocation(prepJfaShader, "uSceneMap"), sceneBuf.texture);
      DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
    EndShaderMode();
  EndTextureMode();

  // ping-pong buffering
  // alternate between two buffers so that we can implement a recursive shader
  // see https://mini.gmshaders.com/p/gm-shaders-mini-recursive-shaders-1308459
  for (int j = jfaSteps*2; j >= 1; j /= 2) {
    jfaBufferC = jfaBufferA;
    jfaBufferA = jfaBufferB;
    jfaBufferB = jfaBufferC;

    BeginTextureMode(jfaBufferA);
      ClearBackground(BLANK);
      BeginShaderMode(jfaShader);
        SetShaderValueTexture(jfaShader, GetShaderLocation(jfaShader, "uCanvas"), jfaBufferB.texture);
        SetShaderValue(jfaShader, GetShaderLocation(jfaShader, "uJumpSize"), &j, SHADER_UNIFORM_INT);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
      EndShaderMode();
    EndTextureMode();
  }

  // write distance field to another buffer
  // reduces strain as the cpu gets to send less data to the gpu for lighting shaders
  BeginTextureMode(distFieldBuf);
    ClearBackground(BLANK);
    BeginShaderMode(distFieldShader);
      SetShaderValueTexture(distFieldShader, GetShaderLocation(distFieldShader, "uJFA"), jfaBufferA.texture);
      DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
    EndShaderMode();
  EndTextureMode();

  // -------------------------------- lighting pass

  int srgbInt = srgb;

  // --------------- traditional GI

  if (gi) {
    int giNoiseInt = giNoise;
    int ambientInt = ambient;
    BeginTextureMode(radianceBufferA);
      BeginShaderMode(giShader);
        ClearBackground(BLANK);
        SetShaderValueTexture(giShader, GetShaderLocation(giShader, "uDistanceField"), distFieldBuf.texture);
        SetShaderValueTexture(giShader, GetShaderLocation(giShader, "uSceneMap"),      sceneBuf.texture);
        SetShaderValueTexture(giShader, GetShaderLocation(giShader, "uLastFrame"),     lastFrameBuf.texture);
        SetShaderValue(giShader, GetShaderLocation(giShader, "uAmbient"),             &ambientInt,          SHADER_UNIFORM_INT);
        SetShaderValue(giShader, GetShaderLocation(giShader, "uAmbientColor"),        &ambientColor,        SHADER_UNIFORM_VEC3);
        SetShaderValue(giShader, GetShaderLocation(giShader, "uRayCount"),  &giRayCount,  SHADER_UNIFORM_INT);
        SetShaderValue(giShader, GetShaderLocation(giShader, "uSrgb"),       &srgbInt,     SHADER_UNIFORM_INT);
        SetShaderValue(giShader, GetShaderLocation(giShader, "uNoise"),      &giNoiseInt,  SHADER_UNIFORM_INT);
        SetShaderValue(giShader, GetShaderLocation(giShader, "uPropagationRate"),  &propagationRate, SHADER_UNIFORM_FLOAT);
        SetShaderValue(giShader, GetShaderLocation(giShader, "uMixFactor"),  &mixFactor, SHADER_UNIFORM_FLOAT);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
      EndShaderMode();
    EndTextureMode();
  } else {

  // --------------- radiance cascades

    if (rcBilinear) {
      SetTextureFilter(radianceBufferA.texture, TEXTURE_FILTER_BILINEAR);
      SetTextureFilter(radianceBufferB.texture, TEXTURE_FILTER_BILINEAR);
      SetTextureFilter(radianceBufferC.texture, TEXTURE_FILTER_BILINEAR);
    } else {
      SetTextureFilter(radianceBufferA.texture, TEXTURE_FILTER_POINT);
      SetTextureFilter(radianceBufferB.texture, TEXTURE_FILTER_POINT);
      SetTextureFilter(radianceBufferC.texture, TEXTURE_FILTER_POINT);
    }

  // --------------- direct lighting pass

    int directDisplayIndex = 0;
    srgbInt = 0;
    int uMixFactor = 0;
    int rcDisableMergingInt = 0;
    int ambientInt = 0;
    for (int i = cascadeAmount; i >= 0; i--) {
      radianceBufferC = radianceBufferA;
      radianceBufferA = radianceBufferB;
      radianceBufferB = radianceBufferC;

      BeginTextureMode(radianceBufferA);
        BeginShaderMode(rcShader);
          ClearBackground(BLANK);
          SetShaderValueTexture(rcShader, GetShaderLocation(rcShader, "uDistanceField"),  distFieldBuf.texture);
          SetShaderValueTexture(rcShader, GetShaderLocation(rcShader, "uSceneMap"),       sceneBuf.texture);
          SetShaderValueTexture(rcShader, GetShaderLocation(rcShader, "uLastPass"),       radianceBufferC.texture);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uAmbient"),             &ambientInt,          SHADER_UNIFORM_INT);
          // SetShaderValue(rcShader, GetShaderLocation(rcShader, "uAmbientColor"),        &ambientColor,        SHADER_UNIFORM_VEC3);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uResolution"),          &resolution,          SHADER_UNIFORM_VEC2);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uBaseRayCount"),        &rcRayCount,          SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uBaseInterval"),        &baseInterval,        SHADER_UNIFORM_FLOAT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uDisableMerging"),      &rcDisableMergingInt, SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uCascadeDisplayIndex"), &directDisplayIndex,  SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uCascadeIndex"),        &i,                   SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uCascadeAmount"),       &cascadeAmount,       SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uSrgb"),                &srgbInt,             SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uMixFactor"),           &uMixFactor,          SHADER_UNIFORM_FLOAT);
          DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
        EndShaderMode();
      EndTextureMode();
    }

    BeginTextureMode(lastFrameBuf);
      DrawTextureRec(radianceBufferA.texture, {0, 0.0, (float)GetScreenWidth(), (float)GetScreenHeight()}, {0.0, 0.0}, WHITE);
    EndTextureMode();

  // --------------- indirect lighting pass (one bounce)
    rcDisableMergingInt = rcDisableMerging;
    srgbInt = srgb;
    ambientInt = ambient;
    for (int i = cascadeAmount; i >= 0; i--) {
      radianceBufferC = radianceBufferA;
      radianceBufferA = radianceBufferB;
      radianceBufferB = radianceBufferC;

      BeginTextureMode(radianceBufferA);
        BeginShaderMode(rcShader);
          ClearBackground(BLANK);
          SetShaderValueTexture(rcShader, GetShaderLocation(rcShader, "uDistanceField"),  distFieldBuf.texture);
          SetShaderValueTexture(rcShader, GetShaderLocation(rcShader, "uSceneMap"),       sceneBuf.texture);
          SetShaderValueTexture(rcShader, GetShaderLocation(rcShader, "uDirectLighting"), lastFrameBuf.texture);
          SetShaderValueTexture(rcShader, GetShaderLocation(rcShader, "uLastPass"),       radianceBufferC.texture);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uAmbient"),             &ambientInt,          SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uAmbientColor"),        &ambientColor,        SHADER_UNIFORM_VEC3);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uResolution"),          &resolution,          SHADER_UNIFORM_VEC2);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uBaseRayCount"),        &rcRayCount,          SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uBaseInterval"),        &baseInterval,        SHADER_UNIFORM_FLOAT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uDisableMerging"),      &rcDisableMergingInt, SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uCascadeDisplayIndex"), &cascadeDisplayIndex, SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uCascadeIndex"),        &i,                   SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uCascadeAmount"),       &cascadeAmount,       SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uSrgb"),                &srgbInt,             SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uPropagationRate"),     &propagationRate,     SHADER_UNIFORM_FLOAT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uMixFactor"),           &mixFactor,           SHADER_UNIFORM_FLOAT);
          DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
        EndShaderMode();
      EndTextureMode();
    }
  }

  // -------------------------------- save to lastFrameBuf for next frame (for traditional GI) & display to main framebuffer

  if (gi) {
    BeginTextureMode(lastFrameBuf);
      DrawTextureRec(radianceBufferA.texture, {0, 0.0, (float)GetScreenWidth(), (float)GetScreenHeight()}, {0.0, 0.0}, WHITE);
    EndTextureMode();
  }

  Rectangle rcRect = { 0.0, 0.0, (float)GetScreenWidth(), (float)GetScreenHeight() };
  Rectangle giRect = { 0.0, (float)GetScreenHeight(), (float)GetScreenWidth(), -(float)GetScreenHeight() };
  DrawTextureRec(
    (gi) ? lastFrameBuf.texture : radianceBufferA.texture,
    (gi) ? giRect : rcRect,
    { 0.0, 0.0 },
    WHITE
  );

}

void Demo::resize() {
  setBuffers();
}

void Demo::setBuffers() {
  UnloadRenderTexture(jfaBufferA);
  UnloadRenderTexture(jfaBufferB);
  UnloadRenderTexture(jfaBufferC);
  UnloadRenderTexture(radianceBufferA);
  UnloadRenderTexture(radianceBufferB);
  UnloadRenderTexture(radianceBufferC);
  UnloadRenderTexture(sceneBuf);
  UnloadRenderTexture(distFieldBuf);
  UnloadRenderTexture(lastFrameBuf);
  UnloadRenderTexture(occlusionBuf);
  UnloadRenderTexture(emissionBuf);

  // change bit depth for jfaBufferA, B, & C so that we can encode texture coordinates without losing data
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

  jfaBufferA      = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
  jfaBufferB      = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
  jfaBufferC      = jfaBufferA;
  radianceBufferA = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
  radianceBufferB = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
  radianceBufferC = radianceBufferA;
  sceneBuf        = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
  distFieldBuf    = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
  lastFrameBuf    = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
  emissionBuf     = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
  occlusionBuf    = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

  changeBitDepth(jfaBufferA,   PIXELFORMAT_UNCOMPRESSED_R32G32B32A32);
  changeBitDepth(jfaBufferB,   PIXELFORMAT_UNCOMPRESSED_R32G32B32A32);
  changeBitDepth(jfaBufferC,   PIXELFORMAT_UNCOMPRESSED_R32G32B32A32);
  changeBitDepth(sceneBuf,     PIXELFORMAT_UNCOMPRESSED_R5G5B5A1);
  changeBitDepth(distFieldBuf, PIXELFORMAT_UNCOMPRESSED_R16);
  changeBitDepth(occlusionBuf, PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA);
  changeBitDepth(emissionBuf,  PIXELFORMAT_UNCOMPRESSED_R5G5B5A1);

  BeginTextureMode(occlusionBuf);
    ClearBackground(WHITE);
  EndTextureMode();

  BeginTextureMode(emissionBuf);
    ClearBackground(BLACK);
  EndTextureMode();
}

void Demo::saveCanvas() {
  Image image = LoadImageFromTexture((gi) ? lastFrameBuf.texture : radianceBufferA.texture);
  if (gi) ImageFlipVertical(&image);

  if (!DirectoryExists("screenshots"))
    MakeDirectory("screenshots");

  std::string path = "screenshots/screenshot0.png";
  for (int i = 0; i < 100; i++) {
    path = "screenshots/screenshot" + std::to_string(i) + ".png";
    if (!FileExists(path.c_str())) break;
  }

  ExportImage(image, path.c_str());

  timeSinceScreenshot = 0;
  screenshotWindowData.open = true;
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

