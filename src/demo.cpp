#include "demo.h"

#define DEFAULT_MIX_FACTOR 0.7
#define DEFAULT_PROPAGATION_RATE 1.3

Demo::Demo() {
  // misc settings
  jfaSteps = 512;
  mouseLight = true;
  drawRainbow = false;

  // scene settings
  orbs = false;
  selectedScene = 4; // pillars
  rainbowAnimation = false;

  // general lighting settings
  srgb = true;
  mixFactor = DEFAULT_MIX_FACTOR;
  propagationRate = DEFAULT_PROPAGATION_RATE;
  ambient = false;
  ambientColor = Vector3(1.0, 1.0, 1.0);

  // gi settings
  gi = false;
  giRayCount = 64;
  giNoise = true;

  // radiance cascades settings
  rcRayCount = 4;
  cascadeAmount = 5;
  cascadeDisplayIndex = 0;
  rcBilinear = true;
  rcDisableMerging = false;
  baseInterval = 0.5;

  // user settings
  user.mode = DRAWING;
  userSetRandomColor();

  // UI
  skipUIRendering = false;
  debugShowBuffers = false;
  debug = false;
  displayNumber = 0;
  displayBuffer = &lastFrameBuf;

  ImGui::GetIO().IniFilename = NULL;
  ImGui::LoadIniSettingsFromDisk("imgui.ini");
  HideCursor();

  colorWindowData.flags |= ImGuiWindowFlags_NoResize;
  settingsWindowData.flags |= ImGuiWindowFlags_NoResize;

  // --- LOAD RESOURCES

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

// void Demo::update() {
// }

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
  #ifdef __APPLE__
  const Shader& drawShader      = shaders["draw_macos.frag"];
  #else
  const Shader& drawShader      = shaders["draw.frag"];
  #endif

  // uniforms
  Vector2 resolution = { (float)GetScreenWidth(), (float)GetScreenHeight() };
  float   time = GetTime();
  Vector2 mousePos = GetMousePosition();
  int     mouseDown = (IsMouseButtonDown(0) || IsMouseButtonDown(1)) && !ImGui::GetIO().WantCaptureMouse;
  int     rainbowAnimationInt = rainbowAnimation;
  int     drawRainbowInt = (IsMouseButtonDown(1) || IsKeyDown(KEY_LEFT_SHIFT)) ? 0 : drawRainbow;
  Vector4 color;

  if (user.mode == DRAWING)
    color = (IsMouseButtonDown(1) || IsKeyDown(KEY_LEFT_SHIFT)) ? Vector4{1.0, 1.0, 1.0, 1.0} : Vector4{0.0, 0.0, 0.0, 1.0};
  else // user.mode == LIGHTING
    color = (IsMouseButtonDown(1) || IsKeyDown(KEY_LEFT_SHIFT)) ? Vector4{0.0, 0.0, 0.0, 1.0} : ColorNormalize(user.brushColor);

  // -------------------------------- scene mapping

  #ifdef __APPLE__
  Texture2D canvas = (user.mode == DRAWING) ? occlusionBuf.texture : emissionBuf.texture;
  #endif

  // drawing to emission or occlusion map depending on `user.mode`
  BeginTextureMode((user.mode == DRAWING) ? occlusionBuf : emissionBuf);
    BeginShaderMode(drawShader);
      #ifdef __APPLE__
      SetShaderValueTexture(drawShader, GetShaderLocation(drawShader, "uCanvas"), canvas);
      #endif
      SetShaderValue(drawShader, GetShaderLocation(drawShader, "uTime"),         &time,           SHADER_UNIFORM_FLOAT);
      SetShaderValue(drawShader, GetShaderLocation(drawShader, "uMousePos"),     &mousePos,       SHADER_UNIFORM_VEC2);
      SetShaderValue(drawShader, GetShaderLocation(drawShader, "uLastMousePos"), &lastMousePos,   SHADER_UNIFORM_VEC2);
      SetShaderValue(drawShader, GetShaderLocation(drawShader, "uBrushSize"),    &user.brushSize, SHADER_UNIFORM_FLOAT);
      SetShaderValue(drawShader, GetShaderLocation(drawShader, "uBrushColor"),   &color,          SHADER_UNIFORM_VEC4);
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
      color = (user.mode == DRAWING) ? Vector4{0.0, 0.0, 0.0, 1.0} : ColorNormalize(user.brushColor);
      int orbsInt = orbs;
      int mouseLightInt = mouseLight && !ImGui::GetIO().WantCaptureMouse;
      SetShaderValueTexture(scenePrepShader, GetShaderLocation(scenePrepShader, "uOcclusionMap"),    occlusionBuf.texture);
      SetShaderValueTexture(scenePrepShader, GetShaderLocation(scenePrepShader, "uEmissionMap"),     emissionBuf.texture);
      SetShaderValue(scenePrepShader, GetShaderLocation(scenePrepShader, "uMousePos"),   &mousePos,            SHADER_UNIFORM_VEC2);
      SetShaderValue(scenePrepShader, GetShaderLocation(scenePrepShader, "uBrushSize"),  &user.brushSize,      SHADER_UNIFORM_FLOAT);
      SetShaderValue(scenePrepShader, GetShaderLocation(scenePrepShader, "uBrushColor"), &color,               SHADER_UNIFORM_VEC4);
      SetShaderValue(scenePrepShader, GetShaderLocation(scenePrepShader, "uMouseLight"), &mouseLightInt,       SHADER_UNIFORM_INT);
      SetShaderValue(scenePrepShader, GetShaderLocation(scenePrepShader, "uTime"),       &time,                SHADER_UNIFORM_FLOAT);
      SetShaderValue(scenePrepShader, GetShaderLocation(scenePrepShader, "uOrbs"),       &orbsInt,             SHADER_UNIFORM_INT);
      SetShaderValue(scenePrepShader, GetShaderLocation(scenePrepShader, "uRainbow"),    &rainbowAnimationInt, SHADER_UNIFORM_INT);
      DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
    EndShaderMode();
  EndTextureMode();

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

  if (!mouseLight) {
    DrawTextureEx(user.brushTexture,
                  Vector2{ (float)(GetMouseX() - user.brushTexture.width  / 2 * user.brushSize),
                           (float)(GetMouseY() - user.brushTexture.height / 2 * user.brushSize) },
                  0.0,
                  user.brushSize,
                  Color{ 0, 0, 0, 128} );
  }
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Demo::renderUI() {
  if (debug) ImGui::ShowDemoWindow();
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

  // -------------------------------- info window

  // imgui's default BulletTextWrapped() function does not wrap
  #define BULLET(x) ImGui::Bullet(); ImGui::TextWrapped(x)
  #define WINDOW_OPACITY 0.35
  #define BASE_INTERVAL_SLIDER() if (ImGui::SmallButton("reset base interval")) { \
                                   baseInterval = 0.5; \
                                 } \
                                 ImGui::SliderFloat("base interval size",     &baseInterval, 0, 64.0, "%.2fpx"); \
                                 ImGui::SetItemTooltip("Radiance interval is used to segment rays.\nThe base interval is for the first casacade\nand is exponentiated per cascade\ne.g. 1px, 2px, 4px, 16px, 64px...\nSee the tutorial for more information.")
  #define BILINEAR_INTERPOLATION_TOGGLE() ImGui::Checkbox("bilinear interpolation", &rcBilinear); \
                                          ImGui::SetItemTooltip("Light further away is drawn at a lower resolution\nto save rendering times. Interpolating\nbetween these pixels makes it look smoother.")
  #define DISPLAY_CASCADE_SLIDER() ImGui::SliderInt("display cascade",     &cascadeDisplayIndex, 0, cascadeAmount-1, "%i"); \
                                   ImGui::SetItemTooltip("The radiance cascade to display; seeing cascades\nindividually can help build intuition over how\nthe algorithm works.\nSee the tutorial for more information.")
  #define DISABLE_MERGING_TOGGLE() ImGui::Checkbox("disable merging", &rcDisableMerging); \
                                   ImGui::SetItemTooltip("each cascade is merged (cascaded) with\nthe cascade above it to form the\nfinal lighting solution.\nSee the tutorial for more information.")

  ImGui::SetNextWindowBgAlpha(WINDOW_OPACITY); // Transparent background
  if (ImGui::Begin("Colour Picker", &colorWindowData.open, colorWindowData.flags)) {
    if (ImGui::SmallButton("set r(a)ndom colour")) userSetRandomColor();
    ImGui::Checkbox("draw rainbow", &drawRainbow);

    Vector4 col4 = ColorNormalize(user.brushColor);
    float col[3] = { col4.x, col4.y, col4.z };
    ImGui::ColorPicker3("##light color", col);
    user.brushColor = ColorFromNormalized(Vector4{col[0], col[1], col[2], 1.0});
    ImGui::End();
  }

  ImGui::SetNextWindowBgAlpha(WINDOW_OPACITY); // Transparent background
  if (ImGui::Begin("Settings", &settingsWindowData.open, settingsWindowData.flags))
  {
    ImGui::TextWrapped("avg frame time %f ms\n(%d fps)", GetFrameTime(), GetFPS());
    if (ImGui::BeginTabBar("lighting scene settings", ImGuiTabBarFlags_None)) {
      if (ImGui::BeginTabItem("Lighting")) {
        if (ImGui::SmallButton("reset settings")) {
          drawRainbow = false;

          // scene settings
          orbs = false;
          selectedScene = 4; // pillars
          rainbowAnimation = false;

          // general lighting settings
          srgb = true;
          mixFactor = DEFAULT_MIX_FACTOR;
          propagationRate = DEFAULT_PROPAGATION_RATE;
          ambient = false;

          // gi settings
          gi = false;
          giRayCount = 64;
          giNoise = true;

          // radiance cascades settings
          cascadeAmount = 5;
          cascadeDisplayIndex = 0;
          rcBilinear = true;
          rcDisableMerging = false;
          baseInterval = 0.5;
        }

        int giInt = gi;
        ImGui::RadioButton("radiance cascades", &giInt, 0);
        ImGui::RadioButton("traditional algorithm", &giInt, 1);
        gi = giInt;

        if (ImGui::CollapsingHeader("General Settings")) {
          ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5);
          ImGui::SliderFloat("mix factor", &mixFactor, 0.0, 1.0, "%.2f");
          ImGui::SetItemTooltip("How much indirect lighting should be present in the scene.");

          ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5);
          ImGui::SliderFloat("propagation", &propagationRate, 0.0, 2.0, "%.2f");
          ImGui::SetItemTooltip("Indirect lighting is multiplied by this value.");

          ImGui::Checkbox("ambient light", &ambient);
          ImGui::SetItemTooltip("Adds a baseline light level to the scene.");

          if (ambient) {
            float col[3] = { ambientColor.x, ambientColor.y, ambientColor.z };
            ImGui::ColorPicker3("##ambient color", col);
            ambientColor = {col[0], col[1], col[2]};
          }

          // ImGui::Checkbox("sRGB conversion", &srgb);
        }

        if (ImGui::CollapsingHeader("Algorithm Settings")) {
          if (gi) {
            ImGui::Checkbox("noise", &giNoise);
            ImGui::SetItemTooltip("Mixes noise into the lighting calculation\nso that a lower ray count can be used\nin exchange for a visually noisier output.");
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
            ImGui::SliderInt("ray count", &giRayCount, 0, 512, "%i");
            ImGui::SetItemTooltip("Amount of rays cast per pixel.");
          } else {
            auto setParams = [this](int n){
              rcRayCount = pow(4, n);
            };

            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
            DISPLAY_CASCADE_SLIDER();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
            BASE_INTERVAL_SLIDER();
            BILINEAR_INTERPOLATION_TOGGLE();
            DISABLE_MERGING_TOGGLE();
          }
        }
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Scene")) {
        const char* scenes[] = { "maze", "trees", "penumbra", "penumbra 2", "pillars", "grid"};

        if (ImGui::Button("select scene"))
            ImGui::OpenPopup("scene_select");

        ImGui::SameLine();
        ImGui::TextUnformatted(selectedScene < 0 ? "<none>" : scenes[selectedScene]);
        if (ImGui::BeginPopup("scene_select")) {
          for (int i = 0; i < IM_ARRAYSIZE(scenes); i++) {
            if (ImGui::Selectable(scenes[i])) {
              selectedScene = i;
              setScene(selectedScene);
            }
          }
          ImGui::EndPopup();
        }

        ImGui::NewLine();

        ImGui::Checkbox("rainbow animation", &rainbowAnimation);
        ImGui::SetItemTooltip("Modulates the hue of emitters.");
        ImGui::Checkbox("light orb circle", &orbs);
        ImGui::SetItemTooltip("Draws some animated circles.");
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
    ImGui::End();
  }

  ImGui::SetNextWindowBgAlpha(WINDOW_OPACITY); // Transparent background
  if (ImGui::Begin("Info/Tutorial", &infoWindowData.open, infoWindowData.flags)) {
    if (ImGui::BeginTabBar("tab bar", ImGuiTabBarFlags_None)) {
      if (ImGui::BeginTabItem("About")) {
        ImGui::TextWrapped("Welcome to 'some light painting'!");
        ImGui::TextWrapped("This demo showcases 2D global illumination with radiance cascades, featuring an additional lighting algorithm for comparison.");
        ImGui::TextWrapped("You can switch between these algorithms in the settings window, where you will also find various parameters pertaining to these algorithms.");
        ImGui::TextWrapped("Feel free to read the tutorial for a minor technical walkthrough.");
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Controls")) {
        ImGui::Text("Brushwork:");

        BULLET("LMB to draw");
        BULLET("RMB (or LMB & left shift) to erase");
        BULLET("Scroll to change brush size");

        ImGui::Text("Changing editing modes:");

        BULLET("1 to switch to editing occluders");
        BULLET("2 to switch to editing emitters");
        BULLET("Space or tab to toggle between editing occluders or emitters");

        ImGui::Text("Canvas:");

        BULLET("C to clear emitters or occluders dependant on mode");
        BULLET("R to reset the current scene (scenes can be selected in the scene tab");

        ImGui::Text("Misc:");

        BULLET("F1 to toggle hiding UI");
        BULLET("F2 to save a screenshot of the canvas");
        BULLET("shift-R to reset UI window locations");
        BULLET("A to switch to a random colour");
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Tutorial")) {
        ImGui::PushStyleColor(ImGuiCol_Separator, ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
        if (ImGui::TreeNode("Outline")) {
          ImGui::TextWrapped("Radiance cascades are a novel data structure used to create global illumination.");

          ImGui::TextWrapped("Global illumination can be defined as lighting where indirect lighting is present. Indirect lighting is lighting that has bounced off at least one surface before arriving to the viewer's eyes or camera. Light is direct when a straight line can be drawn from the light to the light source.");

          ImGui::TextWrapped("Due to light simulation being so expensive, it is hard to produce real-time global illumination that is performant for applications such as video games.");

          ImGui::TextWrapped("Radiance cascades are a novel data structure for global illumination which intends to circumvent one of the biggest constrains on producing performant real-time global illumination - ray count.");

          ImGui::TreePop();
        }

        ImGui::Separator();
        if (ImGui::TreeNode("Background")) {
          ImGui::TextWrapped("Going forward, it is important to note that 'rays' do not refer to light rays, but rays that sample the environment. Rays are used to gather information on the nearest surfaces from a point.");

          ImGui::TextWrapped("The traditional lighting algorithm in this demo is simple: it works by casting a number of rays for each pixel. These rays are then added together to produce the final light value.");

          rlImGuiImageSizeV(&UI_0, {160, 160});

          ImGui::TextWrapped("Where the blue lines indicate a ray, and the yellow rectangle is a pixel.");

          ImGui::TreePop();
        }

        ImGui::Separator();
        if (ImGui::TreeNode("Ray Count")) {
          ImGui::TextWrapped("Radiance cascades builds off the traditional algorithm by realising that ray count can be correlated with distance.");

          rlImGuiImageSizeV(&UI_1, {160, 160});

          ImGui::TextWrapped("Where the white circle is an emitter, the figure above shows a dramatic reduction in ray count from the previous diagram (16 -> 4) while the light source can still be resolved by the pixel as it is close enough.");

          rlImGuiImageSizeV(&UI_2, {160, 160});

          ImGui::TextWrapped("However, once the light source is more distant, 4 rays becomes insufficient for the pixel to find the light source. A higher ray count is required.");

          rlImGuiImageSizeV(&UI_3, {160, 160});

          ImGui::TreePop();
        }

        ImGui::Separator();
        if (ImGui::TreeNode("Radiance Interval")) {
          ImGui::TextWrapped("Radiance interval is therefore defined as a way to explicitly define thresholds for changes in ray count. Radiance interval is measured in pixels.");

          rlImGuiImageSizeV(&UI_4, {160, 160});

          ImGui::TextWrapped("Using radiance interval, we can explicitly determine when the ray count should change. It is optimal for radiance intervals to increase exponentially, meaning they double after each iteration. For example, a base interval size of 0.5 pixels would result in a series of radiance intervals spanning 0.5, 1.0, 2.0, 4.0, 8.0, etc.");

          ImGui::TextWrapped("Interact with the slider below to observe radiance interval in this demo. This is best shown with a small light source, otherwise it may just look noisy.");

          if (ImGui::SmallButton("show small light source")) {
            selectedScene = -2;
            setScene(-2);
          }
          ImGui::SameLine();
          BASE_INTERVAL_SLIDER();

          ImGui::TreePop();
        }

        ImGui::Separator();
        if (ImGui::TreeNode("Pixel Resolution")) {
          ImGui::TextWrapped("While ray count increases, so does computational cost. This means the more distant a pixel is from a light source, the more computation it takes to resolve due to the ray count increasing.");

          ImGui::TextWrapped("We can offset this cost by reducing pixel resolution while we increase ray count - effectively cancelling out the added cost of these additional rays.");

          ImGui::TextWrapped("This is also possible due to the natural fact that light tends to blur the farther it is from a surface. This is best shown with a penumbra.");

          if (ImGui::SmallButton("show penumbra")) {
            selectedScene = 2;
            setScene(2);
          }

          rlImGuiImageSizeV(&UI_5A, {160, 160});

          ImGui::TextWrapped("Note the sharper shadows at the top, and how they blur out. This observation allows us to reduce the fidelity of lighting the farther away it is, whilst having our lighting still appear natural");

          ImGui::TextWrapped("This is how radiance cascades is able to perform better than the traditional algorithm. As ray count is increased, pixel resolution is decreased - all in accordance with radiance interval. We can then simply interpolate between light values so that our light does not look pixelated.");

          ImGui::TextWrapped("Pixel resolution can be directly observed when interpolation is turned off.");

          rlImGuiImageSizeV(&UI_5, {160, 160});

          BILINEAR_INTERPOLATION_TOGGLE();

          ImGui::TreePop();
        }

        ImGui::Separator();
        if (ImGui::TreeNode("Radiance Cascades")) {
          ImGui::TextWrapped("A radiance cascade is a section of light in accordance with a certain radiance interval. Each radiance interval has its corresponding cascade. They can be observed using the slider below.");

          rlImGuiImageSizeV(&UI_6, {240, 160});
          rlImGuiImageSizeV(&UI_7, {240, 160});
          rlImGuiImageSizeV(&UI_8, {240, 160});

          ImGui::TextWrapped("See how each cascade encodes a specific radiance interval - and how two of them are segmented into 4 and 16 parts. This segmentation is how pixel resolution is decreased whilst ray count is increased.");

          BILINEAR_INTERPOLATION_TOGGLE();
          BASE_INTERVAL_SLIDER();
          DISPLAY_CASCADE_SLIDER();
          DISABLE_MERGING_TOGGLE();

          ImGui::TreePop();
        }

        ImGui::Separator();
        if (ImGui::TreeNode("Extra Info")) {
          ImGui::TextWrapped("You may have noticed the ringing artifacts on smaller light sources - this is subject to research - radiance cascades is new after all. This error comes from radiance intervals slightly overlapping.");

          ImGui::TextWrapped("Compare with the traditional algorithm - the biggest difference between them is that this implementation of radiance cascades only produces one light bounce. The traditional algorithm produces many more, but this comes at the cost of latency - the traditional algorithm is ran across multiple frames, whilst radiance cascades are produced all within one frame.");

          ImGui::TreePop();
        }
        ImGui::PopStyleColor();
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
    ImGui::End();
  }
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Demo::processKeyboardInput() {
  if (ImGui::GetIO().WantCaptureMouse) return;

  // switching modes
  if (IsKeyPressed(KEY_ONE))   user.mode = DRAWING;
  if (IsKeyPressed(KEY_TWO))   user.mode = LIGHTING;
  if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_TAB)) user.mode = (user.mode == DRAWING) ? LIGHTING : DRAWING;

  // misc
  if (IsKeyPressed(KEY_GRAVE)) debug = !debug;
  if (IsKeyPressed(KEY_F1))    skipUIRendering = !skipUIRendering;
  if (IsKeyPressed(KEY_F2))    saveCanvas();

  // setting settings
  if (IsKeyPressed(KEY_A)) userSetRandomColor();
  if (IsKeyPressed(KEY_C) || IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_DELETE)) setScene(-1); // clear scene depending on mode
  if (IsKeyPressed(KEY_F)) {
    ToggleFullscreen();
    resize();
  }
  if (IsKeyPressed(KEY_R)) {
    if (IsKeyDown(KEY_LEFT_CONTROL)) { // reload shaders
      std::cout << "Reloading shaders." << std::endl;
      for (auto const& [key, val] : shaders)
        loadShader(key);
    } else if (IsKeyDown(KEY_LEFT_SHIFT)) // reload default ui positions
      ImGui::LoadIniSettingsFromDisk("imgui.ini");
    else
      setScene(selectedScene); // reload scene
  }
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Demo::processMouseInput() {
  // we do not want to be affecting the scene when we're clicking on the UI
  if (ImGui::GetIO().WantCaptureMouse) return;

  user.brushSize += GetMouseWheelMove() / 100;
  if      (user.brushSize < 0.05) user.brushSize = 0.05;
  else if (user.brushSize > 1.0)  user.brushSize = 1.0;

  if (framesSinceLastMousePos > 2) {
    lastMousePos = GetMousePosition();
    framesSinceLastMousePos = 0;
  }
  framesSinceLastMousePos++;
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Demo::resize() {
  setBuffers();
  setScene(selectedScene);
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

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

#define DRAW_TEXTURE_STRETCH(file) DrawTexturePro(LoadTexture(file), Rectangle{0, 0, 800, -600}, Rectangle{0, 0, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())}, Vector2{0, 0}, 0.0, WHITE);

// NOTE: negative numbers are not selectable by the user
void Demo::setScene(int scene) {

  // some scenes are drawn by reading an image file, others are drawn directly via shader
  #ifdef __APPLE__
  const auto draw = [this](Vector2& mousePos, Texture2D& canvas, float brushSize = 0) {
    const Shader& drawShader      = shaders["draw_macos.frag"];
  #else
  const auto draw = [this](Vector2& mousePos, float brushSize = 0) {
    const Shader& drawShader      = shaders["draw.frag"];
  #endif
    if (brushSize == 0) brushSize = user.brushSize;

    Vector4 color = ColorNormalize(user.brushColor);
    int mouseDown = 1;
    BeginShaderMode(drawShader);
      #ifdef __APPLE__
      SetShaderValueTexture(drawShader, GetShaderLocation(drawShader, "uCanvas"), canvas);
      #endif
      SetShaderValue(drawShader, GetShaderLocation(drawShader, "uMousePos"),     &mousePos,  SHADER_UNIFORM_VEC2);
      SetShaderValue(drawShader, GetShaderLocation(drawShader, "uLastMousePos"), &mousePos,  SHADER_UNIFORM_VEC2);
      SetShaderValue(drawShader, GetShaderLocation(drawShader, "uBrushSize"),    &brushSize, SHADER_UNIFORM_FLOAT);
      SetShaderValue(drawShader, GetShaderLocation(drawShader, "uBrushColor"),   &color,     SHADER_UNIFORM_VEC4);
      SetShaderValue(drawShader, GetShaderLocation(drawShader, "uMouseDown"),    &mouseDown, SHADER_UNIFORM_INT);
      DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
    EndShaderMode();
  };

  switch (scene) {
    case -2: { // SMALL LIGHT SOURCE (only available thru UI button in walkthrough)
      BeginTextureMode(occlusionBuf);
        ClearBackground(WHITE);
      EndTextureMode();
      BeginTextureMode(emissionBuf);
        ClearBackground(BLACK);
        Vector2 mousePos = { (float)GetScreenWidth()/2, (float)GetScreenHeight()/2 };
        #ifdef __APPLE__
        draw(mousePos, emissionBuf.texture);
        #else
        draw(mousePos);
        #endif
      EndTextureMode();
      break;
    }
    case -1: // CLEAR
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
    case 0: // MAZE
      BeginTextureMode(occlusionBuf);
        DRAW_TEXTURE_STRETCH("res/textures/scenes/maze.png")
      EndTextureMode();
      break;
    case 1: { // TREES - basically the grid scene but with random offsets
      BeginTextureMode(occlusionBuf);
        ClearBackground(WHITE);
        for (int x = 0; x < GetScreenWidth(); x += GetRandomValue(30, 160)) {
          for (int y = 0; y < GetScreenHeight(); y += GetRandomValue(30, 160)) {
            float xoff = GetRandomValue(-50, 50);
            float yoff = GetRandomValue(10, 50);
            Vector2 mousePos = { x + xoff, y + yoff };
            float brushSize = (float)GetRandomValue(15, 30)/100.0;
            #ifdef __APPLE__
            draw(mousePos, occlusionBuf.texture, brushSize);
            #else
            draw(mousePos, brushSize);
            #endif
          }
        }
      EndTextureMode();
      break;
    }
    case 2: // PENUMBRA
      BeginTextureMode(occlusionBuf);
        DRAW_TEXTURE_STRETCH("res/textures/scenes/penumbra.png")
      EndTextureMode();
      BeginTextureMode(emissionBuf);
        DRAW_TEXTURE_STRETCH("res/textures/scenes/penumbra_e.png")
      EndTextureMode();
      break;
    case 3: // PENUMBRA 2
      BeginTextureMode(occlusionBuf);
        DRAW_TEXTURE_STRETCH("res/textures/scenes/penumbra2.png")
      EndTextureMode();
      BeginTextureMode(emissionBuf);
        DRAW_TEXTURE_STRETCH("res/textures/scenes/penumbra2_e.png")
      EndTextureMode();
      break;
    case 4: { // PILLARS
      BeginTextureMode(occlusionBuf);
        ClearBackground(WHITE);
      EndTextureMode();
      BeginTextureMode(emissionBuf);
        ClearBackground(BLACK);
        #define PILLAR_AMOUNT 4
        float padding = (float)GetScreenWidth() / PILLAR_AMOUNT / 2;
        for (int i = 0; i < PILLAR_AMOUNT; i++) {
          Vector2 mousePos = { (float)GetScreenWidth()/PILLAR_AMOUNT*i + padding, (float)GetScreenHeight()/2 };
          #ifdef __APPLE__
          draw(mousePos, emissionBuf.texture);
          #else
          draw(mousePos);
          #endif
        }
      EndTextureMode();
      break;
    }
    case 5: { // GRID
      BeginTextureMode(occlusionBuf);
        ClearBackground(WHITE);
        for (float x = 0; x < GetScreenWidth(); x += user.brushSize*256*1.5) {
          for (float y = 0; y < GetScreenHeight(); y += user.brushSize*256*1.5) {
            Vector2 mousePos = { x, y };
            #ifdef __APPLE__
            draw(mousePos, occlusionBuf.texture);
            #else
            draw(mousePos);
            #endif
          }
        }
      EndTextureMode();
      break;
    }
  }
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
}
