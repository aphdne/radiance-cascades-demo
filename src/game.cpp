#include "game.h"

void Game::setup() {
  boxPosition = { (float)SCREEN_WIDTH/2, (float)SCREEN_HEIGHT/2 };
  boxSize = 50;
  shader = LoadShader(0, TextFormat("res/shaders/rainbow.frag", GLSL_VERSION));

  debug = false;
}

void Game::update() {
  if (IsKeyPressed(KEY_F3)) debug = !debug;

  if (IsKeyDown(KEY_W)) boxPosition.y -= 80.0f * GetFrameTime();
  if (IsKeyDown(KEY_A)) boxPosition.x -= 80.0f * GetFrameTime();
  if (IsKeyDown(KEY_S)) boxPosition.y += 80.0f * GetFrameTime();
  if (IsKeyDown(KEY_D)) boxPosition.x += 80.0f * GetFrameTime();

  boxSize = 50 + (sin(GetTime() * 2) * 2);
  time = GetTime();
  SetShaderValue(shader, GetShaderLocation(shader, "uTime"), &time, SHADER_UNIFORM_FLOAT);
}

void Game::render() {
  ClearBackground(RAYWHITE);
  BeginShaderMode(shader);
    DrawRectanglePro((Rectangle){ boxPosition.x, boxPosition.y, boxSize, boxSize }, {boxSize/2, boxSize/2}, 0, MAROON);
  EndShaderMode();

  if (debug) {
    DrawText(TextFormat("%i FPS", GetFPS()), 0, 0, 1, BLACK);
  }
}
