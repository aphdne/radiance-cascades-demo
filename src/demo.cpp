#include "demo.h"

Demo::Demo() {

  // --- MISC PARAMETERS
  jfaSteps = 512;
  selectedScene = -1;
  orbs = false;
  mouseLight = true;
  srgb = true;
  drawRainbow = false;
  rainbowAnimation = false;

  gi = false;
  giRayCount = 64;
  giNoise = true;
  mixFactor = 0.7;
  decayRate = 1.3;

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
  displayNumber = 0;
  displayBuffer = &lastFrameBuf;

  // infoWindowData.flags |= ImGuiWindowFlags_NoScrollbar;
  // infoWindowData.flags |= ImGuiWindowFlags_NoResize;
  // infoWindowData.flags |= ImGuiWindowFlags_NoBackground;
  // infoWindowData.flags |= ImGuiWindowFlags_NoInputs;
  // infoWindowData.flags |= ImGuiWindowFlags_NoTitleBar;

  ImGui::GetIO().IniFilename = NULL;
  ImGui::LoadIniSettingsFromDisk("imgui.ini");
  HideCursor();

  // --- LOAD RESOURCES

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
    BeginTextureMode(radianceBufferA);
      BeginShaderMode(giShader);
        ClearBackground(BLANK);
        SetShaderValueTexture(giShader, GetShaderLocation(giShader, "uDistanceField"), distFieldBuf.texture);
        SetShaderValueTexture(giShader, GetShaderLocation(giShader, "uSceneMap"),      sceneBuf.texture);
        SetShaderValueTexture(giShader, GetShaderLocation(giShader, "uLastFrame"),     lastFrameBuf.texture);
        SetShaderValue(giShader, GetShaderLocation(giShader, "uRayCount"),  &giRayCount,  SHADER_UNIFORM_INT);
        SetShaderValue(giShader, GetShaderLocation(giShader, "uSrgb"),       &srgbInt,     SHADER_UNIFORM_INT);
        SetShaderValue(giShader, GetShaderLocation(giShader, "uNoise"),      &giNoiseInt,  SHADER_UNIFORM_INT);
        SetShaderValue(giShader, GetShaderLocation(giShader, "uDecayRate"),  &decayRate, SHADER_UNIFORM_FLOAT);
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
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uResolution"),          &resolution,          SHADER_UNIFORM_VEC2);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uBaseRayCount"),        &rcRayCount,          SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uBaseInterval"),        &baseInterval,        SHADER_UNIFORM_FLOAT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uDisableMerging"),      &rcDisableMergingInt, SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uCascadeDisplayIndex"), &cascadeDisplayIndex, SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uCascadeIndex"),        &i,                   SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uCascadeAmount"),       &cascadeAmount,       SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uSrgb"),                &srgbInt,             SHADER_UNIFORM_INT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uDecayRate"),           &decayRate,         SHADER_UNIFORM_FLOAT);
          SetShaderValue(rcShader, GetShaderLocation(rcShader, "uMixFactor"),           &mixFactor, SHADER_UNIFORM_FLOAT);
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

  // imgui's default BulletText() function does not wrap
  #define BULLET(x) ImGui::Bullet(); ImGui::TextWrapped(x)

  if (!ImGui::Begin("Radiance Cascades", &infoWindowData.open, infoWindowData.flags)) {
    ImGui::End();
  } else {
    ImGui::Text("average frame time %f ms (%d fps)", GetFrameTime(), GetFPS());
    std::string str = "built ";
    str += __DATE__;
    str += " at ";
    str += __TIME__;
    ImGui::Text(str.c_str());

    if (ImGui::BeginTabBar("tab bar", ImGuiTabBarFlags_None)) {

      if (ImGui::BeginTabItem("Info & Controls")) {
        if (ImGui::CollapsingHeader("Read Me!")) {
          ImGui::SeparatorText("ABOUT:");
          BULLET("welcome to the radiance cascades demo 'some light painting'!");
          BULLET("this demo showcases 2D global illumination (GI), which can trivially be defined as realistic lighting");
          BULLET("this demo features a traditional GI algorithm alongside radiance cascades for comparison");
          BULLET("parameters can be altered in the lighting tab and show an explanatory tooltip when moused over");
          ImGui::SeparatorText("CONTROLS:");
          BULLET("drawing & erasing:");
          ImGui::Indent();
            BULLET("LMB to draw");
            BULLET("RMB (or LMB & left shift) to erase");
            BULLET("scroll to change brush size");
            BULLET("1 to switch to editing occluder");
            BULLET("2 to switch to editing emitters");
            BULLET("C to clear emitters or occluders depending on mode");
            BULLET("R to reset scene - scenes can be selected in the scene tab");
          ImGui::Unindent();
          BULLET("other:");
          ImGui::Indent();
            BULLET("A to switch to a random colour");
            BULLET("F1 to toggle hiding UI");
            BULLET("sliders can be inputted via keyboard by clicking with cmd/ctrl");
          ImGui::Unindent();
        }
        if (ImGui::CollapsingHeader("Explanatory Info")) {
          if (ImGui::TreeNode("Outline")) {
            BULLET("radiance cascades are a novel data structure used to create performant realistic lighting\n");
            BULLET("radiance cascades work by reducing the pixel resolution of lighting the farther it is from a light source and using linear interpolation between light values to make up the difference");

            ImGui::TreePop();
          }
          if (ImGui::TreeNode("Background")) {
            BULLET("the traditional lighting algorithm works by casting N amount of rays per pixel to sample the environment, adding all the results of those samples together, and dividing by N");

            ImGui::NewLine();

            BULLET("radiance cascades builds off this algorithm with two key observations:");
            ImGui::Indent();
              BULLET("a pixel up close to a light source does not need as many rays as one farther away");
              BULLET("pixels farther from a light source do not need as much spatial resolution as pixels closer to a light source");
            ImGui::Unindent();

            ImGui::TreePop();
          }

          if (ImGui::TreeNode("Ray Count (Angular Resolution)")) {
            BULLET("ray count refers to the amount of rays being cast per pixel");
            BULLET("in the traditional algorithm this number does not change - it is the same number for every pixel");

            ImGui::NewLine();

            BULLET("in radiance cascades this is altered to be increased depending on distance to the nearest light source");
            BULLET("so that pixels close to a light source do not cast an unneccessary amount of rays, whilst pixels farther away continue to cast as many as they need to accurately resolve the light source");

            ImGui::NewLine();

            BULLET("'radiance interval' is therefore used as a discrete way of dividing raycount");
            BULLET("interact with the slider below to see how radiance interval correlates with distance:");
            ImGui::SliderFloat("base interval size",     &baseInterval, 0, 64.0, "%.2fpx");
            ImGui::SetItemTooltip("radiance interval is used to segment rays\nthe base interval is for the first casacade\nand is exponentiated per cascade\ne.g. 1px, 2px, 4px, 16px, 64px...");
            BULLET("(this is best viewed with a small light source)");

            ImGui::NewLine();

            BULLET("radiance interval increases expontentially so that at a base interval of 1px it will increase to 2px, then 4px, 16px, 64px, etc.");

            ImGui::TreePop();
          }

          if (ImGui::TreeNode("Linear/Spatial Resolution")) {
            BULLET("when viewing radiance interval it is easy to observe that where raycount increases there is a blurring effect");
            BULLET("this is the effect of decreasing spatial/linear resolution - ray count and spatial/linear resolution are inversely correlated");
            BULLET("this can be best observed by toggling bilinear interpolation - see the pixelated effect distant from light sources.");

            ImGui::NewLine();

            ImGui::Checkbox("bilinear interpolation", &rcBilinear);
            ImGui::SetItemTooltip("as higher cascades have higher segments (probes)\ntheir output will be more pixelated. interpolating\nthese outputs makes them feasible\nas a lighting solution");

            ImGui::NewLine();

            BULLET("this is done by dividing the viewport into segments and dedicating certain ray angles to those segments, effectively cancelling out the increased performance cost of a higher ray count");
            BULLET("these segments are then overlayed over one another (merged) to create the scene's lighting - hence radiance *cascades*");

            ImGui::NewLine();

            BULLET("this can be directly observed by interacting with the 'display cascade' parameter and optionally disabling merging");
            BULLET("an exaggerated base interval can also help with observation");
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
            ImGui::SliderInt("display cascade", &cascadeDisplayIndex, 0, cascadeAmount-1, "%i");
            ImGui::Checkbox("disable merging", &rcDisableMerging);
            ImGui::SetItemTooltip("each cascade is merged (cascaded) with\nthe cascade above it to form the\nfinal lighting solution");

            ImGui::TreePop();
          }
        }
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Lighting")) {
        ImGui::TextWrapped("please note a detailed walkthrough is available under the info & controls tab!");

        int giInt = gi;
        ImGui::RadioButton("radiance cascades", &giInt, 0);
        ImGui::RadioButton("traditional algorithm", &giInt, 1);
        gi = giInt;

        if (ImGui::CollapsingHeader("General Settings")) {
          ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5);
          ImGui::SliderFloat("mix factor", &mixFactor, 0.0, 1.0, "%.2f");
          ImGui::SetItemTooltip("percentage: how much the original scene texture\nshould be mixed with the direct lighting pass");

          ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5);
          ImGui::SliderFloat("propagation", &decayRate, 0.0, 2.0, "%.2f");
          ImGui::SetItemTooltip("how much the indirect lighting should be propagated");

          ImGui::Checkbox("sRGB conversion", &srgb);
          ImGui::SetItemTooltip("conversion from linear colour space to \nsRGB colour space; sRGB skews certain \ncolour values so that colours appear more\nnaturally to the human eye");
        }

        if (ImGui::CollapsingHeader("Algorithm Settings")) {
          if (gi) {
            // ImGui::SeparatorText("traditional gi");
            ImGui::Checkbox("noise", &giNoise);
            ImGui::SetItemTooltip("mixes noise into the lighting calculation\nso that a lower ray count can be used\nin exchange for a more noisy output");
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
            ImGui::SliderInt("ray count", &giRayCount, 0, 512, "%i");
            ImGui::SetItemTooltip("amount of rays cast per pixel");
          } else {
            // ImGui::SeparatorText("radiance cascades");

            auto setParams = [this](int n){
              rcRayCount = pow(4, n);
            };

            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
            ImGui::SliderInt("display cascade",     &cascadeDisplayIndex, 0, cascadeAmount-1, "%i");
            ImGui::SetItemTooltip("cascade to display; seeing cascades\nindividually can help build intuition\nover how the algorithm works");
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
            ImGui::SliderFloat("base interval size",     &baseInterval, 0, 64.0, "%.2fpx");
            ImGui::SetItemTooltip("radiance interval is used to segment rays\nthe base interval is for the first casacade\nand is exponentiated per cascade\ne.g. 1px, 2px, 4px, 16px, 64px...");
            ImGui::Checkbox("bilinear interpolation", &rcBilinear);
            ImGui::SetItemTooltip("as higher cascades have higher segments (probes)\ntheir output will be pixelated. interpolating\nthese outputs makes them feasible\nas a lighting solution");
            ImGui::Checkbox("disable merging",        &rcDisableMerging);
            ImGui::SetItemTooltip("each cascade is merged (cascaded) with\nthe cascade above it to form the\nfinal lighting solution");
          }
        }
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Scene")) {
        static bool toggles[] = { true, false, false, false };
        const char* names[] = { "maze", "trees", "penumbra", "penumbra 2"};

        // Simple selection popup (if you want to show the current selection inside the Button itself,
        // you may want to build a string using the "###" operator to preserve a constant ID with a variable label)
        if (ImGui::Button("select scene"))
            ImGui::OpenPopup("scene_select");
        ImGui::SameLine();
        ImGui::TextUnformatted(selectedScene == -1 ? "<none>" : names[selectedScene]);
        if (ImGui::BeginPopup("scene_select")) {
          for (int i = 0; i < IM_ARRAYSIZE(names); i++) {
            if (ImGui::Selectable(names[i])) {
              selectedScene = i;
              setScene(selectedScene);
            }
          }
          if (ImGui::SliderInt("##display stage",   &displayNumber, 0, 3, "display stage = %i")) {
            switch (displayNumber) {
              case 0:
                displayBuffer = &sceneBuf;
                break;
              case 1:
                displayBuffer = &jfaBufferA;
                break;
              case 2:
                displayBuffer = &distFieldBuf;
                break;
              case 3:
                displayBuffer = &lastFrameBuf;
                break;
            }
          }
          Vector2 displaySize = {80.0, (float)80.0*(std::min((float)GetScreenWidth(), (float)GetScreenHeight()) / std::max((float)GetScreenWidth(), (float)GetScreenHeight()))};
          rlImGuiImageSizeV(&displayBuffer->texture, displaySize);

          ImGui::EndPopup();
        }

        ImGui::Checkbox("rainbow animation", &rainbowAnimation);
        ImGui::SetItemTooltip("modulates the hue of emitters, offset by the underlying hue of the emitter itself for interesting animation");
        ImGui::Checkbox("mouse light", &mouseLight);
        ImGui::SetItemTooltip("draws an occluder/emitter on mouse position");
        ImGui::Checkbox("light orb circle", &orbs);
        ImGui::SetItemTooltip("draws some animated circles to light up the scene");
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Colour Picker")) {
        if (ImGui::SmallButton("set r(a)ndom colour")) userSetRandomColor();
        ImGui::Checkbox("draw rainbow", &drawRainbow);
        Vector4 col4 = ColorNormalize(user.brushColor);
        float col[3] = { col4.x, col4.y, col4.z };
        ImGui::ColorPicker3("##light color", col);
        user.brushColor = ColorFromNormalized(Vector4{col[0], col[1], col[2], 1.0});
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
    ImGui::End();
  }
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Demo::processKeyboardInput() {
  if (IsKeyPressed(KEY_ONE))   user.mode = DRAWING;
  if (IsKeyPressed(KEY_TWO))   user.mode = LIGHTING;

  if (IsKeyPressed(KEY_GRAVE)) debug = !debug;
  if (IsKeyPressed(KEY_F1))    skipUIRendering = !skipUIRendering;

  if (IsKeyPressed(KEY_A)) userSetRandomColor();
  if (IsKeyPressed(KEY_C)) setScene(-1);
  if (IsKeyPressed(KEY_R)) {
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
      std::cout << "Reloading shaders." << std::endl;
      for (auto const& [key, val] : shaders) {
        loadShader(key);
      }
    } else if (IsKeyDown(KEY_LEFT_SHIFT)) {
      ImGui::LoadIniSettingsFromDisk("imgui.ini");
    } else {
      setScene(selectedScene);
    }
  }
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Demo::processMouseInput() {
  // we do not want to be affecting the scene when we're clicking on the UI
  if (ImGui::GetIO().WantCaptureMouse) return;

  user.brushSize += GetMouseWheelMove() / 100;
  if      (user.brushSize < 0.05) user.brushSize = 0.05;
  else if (user.brushSize > 1.0)  user.brushSize = 1.0;

  if (framesSinceLastMousePos > 9) {
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

void Demo::setScene(int scene) {
  switch (scene) {
    case -1:
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
    case 0:
      BeginTextureMode(occlusionBuf);
        DRAW_TEXTURE_STRETCH("res/textures/canvas/maze.png")
      EndTextureMode();
      break;
    case 1:
      BeginTextureMode(occlusionBuf);
        DRAW_TEXTURE_STRETCH("res/textures/canvas/trees.png")
      EndTextureMode();
      break;
    case 2:
      BeginTextureMode(occlusionBuf);
        DRAW_TEXTURE_STRETCH("res/textures/canvas/penumbra.png")
      EndTextureMode();
      BeginTextureMode(emissionBuf);
        DRAW_TEXTURE_STRETCH("res/textures/canvas/penumbra_e.png")
      EndTextureMode();
      break;
    case 3:
      BeginTextureMode(occlusionBuf);
        DRAW_TEXTURE_STRETCH("res/textures/canvas/penumbra2.png")
      EndTextureMode();
      BeginTextureMode(emissionBuf);
        DRAW_TEXTURE_STRETCH("res/textures/canvas/penumbra2_e.png")
      EndTextureMode();
      break;
  }
}
