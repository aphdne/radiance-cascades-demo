#include "game.h"

void Game::setup() {
 boxPosition = { (float)SCREEN_WIDTH/2, (float)SCREEN_HEIGHT/2 };
 boxSize = 50;
}

void Game::update() {
  if (IsKeyDown(KEY_D)) boxPosition.x += 80.0f * GetFrameTime();
  if (IsKeyDown(KEY_A)) boxPosition.x -= 80.0f * GetFrameTime();
  if (IsKeyDown(KEY_W)) boxPosition.y -= 80.0f * GetFrameTime();
  if (IsKeyDown(KEY_S)) boxPosition.y += 80.0f * GetFrameTime();

  boxSize = 50 + (sin(GetTime() * 2) * 2);
}

void Game::render() {
  ClearBackground(RAYWHITE);
  DrawRectangleV(boxPosition, Vector2(boxSize, boxSize), MAROON);
}
