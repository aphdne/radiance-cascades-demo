#include "pong_demo.h"

#define PI 3.14159265359
#define HALF_PI (PI/2)

PongDemo::PongDemo() {
  ball.position = { GetScreenWidth()/2, GetScreenHeight()/2 };
  ball.direction = {1, 0};
  ball.size = 24.0;
  ball.color = {1.0, 0.0, 0.0};
  ball.speed = 128.0;

  leftPaddle.position = { GetScreenWidth()/2, GetScreenHeight()/2 };
  leftPaddle.direction = 0;
  leftPaddle.color = {0.0, 0.0, 1.0};

  rightPaddle.position = { GetScreenWidth()/2, GetScreenHeight()/2 };
  rightPaddle.direction = 0;
  rightPaddle.color = {0.0, 1.0, 0.0};
}

void PongDemo::update() {
  ball.position += ball.direction * ball.speed * GetFrameTime();

  // AI
  if (GetTime() - lastAiFrameTime > AI_FRAMERATE) {
    if (rightPaddle.position.y > ball.position.y)
      rightPaddle.direction = 0.0;
    else if (rightPaddle.position.y < ball.position.y)
      rightPaddle.direction = 1.0;
    lastAiFrameTime = GetTime();
  }

  // OUT OF BOUNDS CHECK
  if (ball.position.x - ball.size > GetScreenWidth() || ball.position.x + ball.size < 0)
    resetBall();
  else if (ball.position.y + ball.size > GetScreenHeight() || ball.position.y - ball.size < 0)
    ball.direction.y = -ball.direction.y;

  leftPaddle.position.y += (leftPaddle.direction ? 1 : -1) * paddleSpeed * GetFrameTime();
  if (leftPaddle.position.y + paddleSize.y > GetScreenHeight() || leftPaddle.position.y - paddleSize.y < 0)
    leftPaddle.direction = !leftPaddle.direction;

  rightPaddle.position.y += (rightPaddle.direction ? 1 : -1) * paddleSpeed * GetFrameTime();
  if (rightPaddle.position.y + paddleSize.y > GetScreenHeight() || rightPaddle.position.y - paddleSize.y < 0)
    rightPaddle.direction = !rightPaddle.direction;

  // COLLISION CHECKS
  int halfsize = ball.size;
  Vector2 phalfsize = paddleSize/2;
  if ((ball.position.y + halfsize > leftPaddle.position.y - phalfsize.y &&
       ball.position.y - halfsize < leftPaddle.position.y + phalfsize.y) &&
      (ball.position.x + halfsize > leftPaddle.position.x - phalfsize.x &&
       ball.position.x - halfsize < leftPaddle.position.x + phalfsize.x)) {
    Vector2 dir = Vector2Normalize((ball.position - leftPaddle.position) / paddleSize);
    float angle = std::atan2(dir.x, dir.y) - (HALF_PI / 2);

    if ((angle > 0 && angle <= HALF_PI) || (angle > -PI && angle <= -HALF_PI)) {
      ball.direction.x = -ball.direction.x; // left & right
    } else if ((angle > HALF_PI && angle <= 0)) {
      ball.direction.y = -ball.direction.y; // top & bottom
    }
  }

  if ((ball.position.y + halfsize > rightPaddle.position.y - phalfsize.y &&
       ball.position.y - halfsize < rightPaddle.position.y + phalfsize.y) &&
      (ball.position.x + halfsize > rightPaddle.position.x - phalfsize.x &&
       ball.position.x - halfsize < rightPaddle.position.x + phalfsize.x)) {
    Vector2 dir = Vector2Normalize((ball.position - rightPaddle.position) / paddleSize);
    float angle = std::atan2(dir.x, dir.y) - (HALF_PI / 2);

    if ((angle > 0 && angle <= HALF_PI) || (angle > -PI && angle <= -HALF_PI)) {
      ball.direction.x = -ball.direction.x; // left & right
    } else if ((angle > HALF_PI && angle <= 0)) {
      ball.direction.y = -ball.direction.y; // top & bottom
    }
  }
}

void PongDemo::prepScene() {
  ClearBackground(BLACK);
  const Shader& shader = shaders["pong.frag"];
  Vector2 resolution = { (float)GetScreenWidth(), (float)GetScreenHeight() };

  // combine occlusion & emission map to be used in JFA and lighting passes
  // this shader is also an opportunity to add SDFs (such as the orbs)
  BeginTextureMode(sceneBuf);
    ClearBackground(BLANK);
    BeginShaderMode(shader);
      SetShaderValue(shader, GetShaderLocation(shader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);
      SetShaderValue(shader, GetShaderLocation(shader, "uBall.position"), &ball.position, SHADER_UNIFORM_VEC2);
      SetShaderValue(shader, GetShaderLocation(shader, "uBall.size"),     &ball.size,     SHADER_UNIFORM_INT);
      SetShaderValue(shader, GetShaderLocation(shader, "uBall.color"),    &ball.color,    SHADER_UNIFORM_VEC3);
      SetShaderValue(shader, GetShaderLocation(shader, "uPaddleSize"),          &paddleSize,          SHADER_UNIFORM_VEC2);
      SetShaderValue(shader, GetShaderLocation(shader, "uLeftPaddle.position"), &leftPaddle.position, SHADER_UNIFORM_VEC2);
      SetShaderValue(shader, GetShaderLocation(shader, "uLeftPaddle.color"),    &leftPaddle.color,    SHADER_UNIFORM_VEC3);
      SetShaderValue(shader, GetShaderLocation(shader, "uRightPaddle.position"), &rightPaddle.position, SHADER_UNIFORM_VEC2);
      SetShaderValue(shader, GetShaderLocation(shader, "uRightPaddle.color"),    &rightPaddle.color,    SHADER_UNIFORM_VEC3);
      DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
    EndShaderMode();
  EndTextureMode();
}

void PongDemo::renderUI() {
  Demo::renderUI();

  if (!debug) return;

  ImGui::SetNextWindowBgAlpha(WINDOW_OPACITY); // Transparent background
  if (!ImGui::Begin("Settings", &settingsWindowData.open, settingsWindowData.flags)) {
    ImGui::End();
  } else {
    ImGui::SliderInt("ball size", &ball.size, 0, 512, "%i");
    ImGui::SliderFloat("ball speed", &ball.speed, 0, 512, "%f");
    ImGui::SliderFloat("paddle size x", &paddleSize.x, 16, 512, "%f");
    ImGui::SliderFloat("paddle size y", &paddleSize.y, 64, 512, "%f");
    ImGui::SliderFloat("paddle speed", &paddleSpeed, 0, 512, "%f");
    ImGui::End();
  }
}


void PongDemo::processMouseInput() {
  Demo::processMouseInput();
}

void PongDemo::processKeyboardInput() {
  Demo::processKeyboardInput();

  if (IsKeyPressed(KEY_UP)) {
    leftPaddle.direction = 0;
  }
  if (IsKeyPressed(KEY_DOWN)) {
    leftPaddle.direction = 1;
  }

  if (IsKeyPressed(KEY_W)) {
    rightPaddle.direction = 0;
  }
  if (IsKeyPressed(KEY_S)) {
    rightPaddle.direction = 1;
  }
}

void PongDemo::resize() {
  Demo::resize();

  leftPaddle.position = { GetScreenWidth()/2 - GetScreenWidth()/4, GetScreenHeight()/2 };
  leftPaddle.direction = 0;
  leftPaddle.color = {0.0, 0.0, 1.0};

  rightPaddle.position = { GetScreenWidth()/2 + GetScreenWidth()/4, GetScreenHeight()/2 };
  rightPaddle.direction = 0;
  rightPaddle.color = {0.0, 1.0, 0.0};

  ball.position = { GetScreenWidth()/2, GetScreenHeight()/2 };
}

void PongDemo::resetBall() {
  ball.position = { GetScreenWidth()/2, GetScreenHeight()/2 };
  ball.direction.x += static_cast<float>(GetRandomValue(0, 100))/100;
  ball.direction.y += static_cast<float>(GetRandomValue(0, 100))/100;
  ball.direction = Vector2Normalize(ball.direction);
}
