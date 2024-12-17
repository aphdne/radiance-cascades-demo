#include "game.h"

Game::Game() {
  box.position = { (float)SCREEN_WIDTH/2, (float)SCREEN_HEIGHT/2 };
  box.size = 20;
  debug = false;

  rainbowShader = LoadShader(0, TextFormat("res/shaders/rainbow.frag", GLSL_VERSION));
  maskShader = LoadShader(0, TextFormat("res/shaders/mask.frag", GLSL_VERSION));

  brush.img = LoadImage("res/brush_circle.png");
  brush.tex = LoadTextureFromImage(brush.img);
  brush.scale = 0.25;

  //canvas.img = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  canvas.img = LoadImage("res/maze.png");
  canvas.tex = LoadTextureFromImage(canvas.img);
}

void Game::update() {
  time = GetTime();
  SetShaderValue(rainbowShader, GetShaderLocation(rainbowShader, "uTime"), &time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(maskShader,    GetShaderLocation(maskShader, "uTime"),    &time, SHADER_UNIFORM_FLOAT);

  Vector2 resolution = { SCREEN_WIDTH, SCREEN_HEIGHT };
  SetShaderValue(maskShader, GetShaderLocation(maskShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(maskShader, GetShaderLocation(maskShader, "uLightPos"), &box.position, SHADER_UNIFORM_VEC2);
}

void Game::render() {
  ClearBackground(PINK);

  BeginShaderMode(maskShader);
    // <!> SetShaderValueTexture() has to be called while the shader is enabled
    SetShaderValueTexture(maskShader, GetShaderLocation(maskShader, "uOcclusionMask"), canvas.tex);
    DrawTexture(canvas.tex, 0, 0, WHITE);
  EndShaderMode();

  // BeginShaderMode(rainbowShader);
  //   DrawRectanglePro((Rectangle){ box.position.x, box.position.y, box.size, box.size },
  //                    (Vector2){ box.size/2, box.size/2 },
  //                    0,
  //                    MAROON);
  // EndShaderMode();

  DrawTextureEx(brush.tex,
                (Vector2){ (float)(GetMouseX() - brush.img.width/2*brush.scale),
                           (float)(GetMouseY() - brush.img.height/2*brush.scale) },
                0.0,
                brush.scale,
                BLACK);
}

void Game::renderUI() {
  if (debug) {
    DrawText(TextFormat("%i FPS",    GetFPS()), 0, 8, 1, BLACK);
  }
}

void Game::processKeyboardInput() {
  if (IsKeyPressed(KEY_F3)) debug = !debug;
  if (IsKeyPressed(KEY_C)) {
    std::cout << "Clearing canvas." << std::endl;
    canvas.img = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    UnloadTexture(canvas.tex);
    canvas.tex = LoadTextureFromImage(canvas.img);
  }
  if (IsKeyPressed(KEY_V)) {
    std::cout << "Mazing canvas." << std::endl;
    canvas.img = LoadImage("res/maze.png");
    UnloadTexture(canvas.tex);
    canvas.tex = LoadTextureFromImage(canvas.img);
  }

  if (IsKeyDown(KEY_W)) box.position.y -= 80.0f * GetFrameTime();
  if (IsKeyDown(KEY_A)) box.position.x -= 80.0f * GetFrameTime();
  if (IsKeyDown(KEY_S)) box.position.y += 80.0f * GetFrameTime();
  if (IsKeyDown(KEY_D)) box.position.x += 80.0f * GetFrameTime();
}

void Game::processMouseInput() {
  if (IsMouseButtonDown(0)) {
    ImageDraw(&canvas.img,
              brush.img,
              (Rectangle){ 0, 0, (float)canvas.img.width, (float)canvas.img.height },
              (Rectangle){ static_cast<float>(GetMouseX() - brush.img.width/2*brush.scale),
                           static_cast<float>(GetMouseY() - brush.img.height/2*brush.scale),
                           static_cast<float>(brush.img.width * brush.scale),
                           static_cast<float>(brush.img.height * brush.scale) },
              BLACK);
    UnloadTexture(canvas.tex);
    canvas.tex = LoadTextureFromImage(canvas.img);
  } else if (IsMouseButtonDown(1)) {
    ImageDraw(&canvas.img,
              brush.img,
              (Rectangle){ 0, 0, (float)canvas.img.width, (float)canvas.img.height },
              (Rectangle){ static_cast<float>(GetMouseX() - brush.img.width/2*brush.scale),
                           static_cast<float>(GetMouseY() - brush.img.height/2*brush.scale),
                           static_cast<float>(brush.img.width*brush.scale),
                           static_cast<float>(brush.img.height*brush.scale) },
              WHITE);
    UnloadTexture(canvas.tex);
    canvas.tex = LoadTextureFromImage(canvas.img);
  }
  brush.scale += GetMouseWheelMove()/100;
  if (brush.scale < 0.1) brush.scale = 0.1;
}
