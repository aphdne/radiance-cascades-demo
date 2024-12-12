#include "game.h"

void Game::setup() {
  boxPosition = { (float)SCREEN_WIDTH/2, (float)SCREEN_HEIGHT/2 };
  boxSize = 50;
  shader = LoadShader(0, TextFormat("res/shaders/rainbow.frag", GLSL_VERSION));

  brush = LoadImage("res/brush_circle.png");
  brushTex = LoadTexture("res/brush_circle.png");
  debug = false;

  canvas = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BACKGROUND_COLOR);
  canvasTex = LoadTextureFromImage(canvas);
}



void Game::update() {
  if (IsKeyPressed(KEY_F3)) debug = !debug;
  if (IsKeyPressed(KEY_C)) {
    canvas = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, BACKGROUND_COLOR);
    UnloadTexture(canvasTex);
    canvasTex = LoadTextureFromImage(canvas);
  }

//---

  if (IsKeyDown(KEY_W)) boxPosition.y -= 80.0f * GetFrameTime();
  if (IsKeyDown(KEY_A)) boxPosition.x -= 80.0f * GetFrameTime();
  if (IsKeyDown(KEY_S)) boxPosition.y += 80.0f * GetFrameTime();
  if (IsKeyDown(KEY_D)) boxPosition.x += 80.0f * GetFrameTime();

  boxSize = 50 + (sin(GetTime() * 2) * 2);
  time = GetTime();
  SetShaderValue(shader, GetShaderLocation(shader, "uTime"), &time, SHADER_UNIFORM_FLOAT);

  if (IsMouseButtonDown(0)) {
    // std::cout << GetMouseX() << " -> " << GetMouseX() - brush.width/2*BRUSH_SCALE << std::endl;
    // std::cout << GetMouseY() << " -> " << GetMouseY() - brush.width/2*BRUSH_SCALE << std::endl;
    BeginBlendMode(BLEND_ALPHA);
    for (int x = 0; x < brush.width*BRUSH_SCALE; x++) {
      for (int y = 0; y < brush.height*BRUSH_SCALE; y++) {
        Color brushcol = GetImageColor(brush,  x/BRUSH_SCALE, y/BRUSH_SCALE);
        Color bgcol    = GetImageColor(canvas, x/BRUSH_SCALE, y/BRUSH_SCALE);
        if (brushcol.a != 0) {
          ImageDrawPixel(&canvas,
                         GetMouseX() - brush.width/2*BRUSH_SCALE + x,
                         GetMouseY() - brush.height/2*BRUSH_SCALE + y,
                         // ColorAlphaBlend(bgcol, brushcol, BLACK)); // alpha doesnt work!!
                         ColorAlphaBlend(brushcol, bgcol, BLACK));
        }
      }
    }
    EndBlendMode();
    UnloadTexture(canvasTex);
    canvasTex = LoadTextureFromImage(canvas);
  }
}

void Game::render() {
  ClearBackground(PINK);

  DrawTexture(canvasTex, 0, 0, WHITE);

  BeginShaderMode(shader);
    DrawRectanglePro((Rectangle){ boxPosition.x, boxPosition.y, boxSize, boxSize },
                     (Vector2){ boxSize/2, boxSize/2 },
                     0,
                     MAROON);
  EndShaderMode();

  DrawTextureEx(brushTex,
                (Vector2){ (float)(GetMouseX() - brush.width/2*BRUSH_SCALE),
                           (float)(GetMouseY() - brush.height/2*BRUSH_SCALE) },
                0.0,
                BRUSH_SCALE,
                BLACK);

  if (debug) {
    DrawText(TextFormat("%i FPS", GetFPS()), 0, 0, 1, BLACK);
  }
}
