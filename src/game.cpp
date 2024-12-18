#include "game.h"

Game::Game() {
  box.position = { (float)SCREEN_WIDTH/2, (float)SCREEN_HEIGHT/2 };
  box.size = 20;
  debug = true;
  mode = DRAWING;

  rainbowShader = LoadShader(0, TextFormat("res/shaders/rainbow.frag", GLSL_VERSION));
  lightingShader = LoadShader(0, TextFormat("res/shaders/lighting.frag", GLSL_VERSION));

  brush.img = LoadImage("res/brush_circle.png");
  brush.tex = LoadTextureFromImage(brush.img);
  brush.scale = 0.25;

  canvas.img = LoadImage("res/maze.png");
  canvas.tex = LoadTextureFromImage(canvas.img);

  int N = 4;       // no. of lights
  float d = 256.0; // distance from centre
  for (float i = 0; i < N; i++) {
    float t = (i+1) * (PI*2/N) + 0.1;
    Light l;
    l.position = (Vector2){ SCREEN_WIDTH/2 + std::sin(t) * d, SCREEN_HEIGHT/2 + std::cos(t) * d};
    l.color    = (Vector3){ std::sin(t), std::cos(t), 1.0 };
    l.size     = 600;
    lights.push_back(l);
  }
}

void Game::update() {
  time = GetTime();
  SetShaderValue(rainbowShader, GetShaderLocation(rainbowShader, "uTime"), &time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(lightingShader,    GetShaderLocation(lightingShader, "uTime"),    &time, SHADER_UNIFORM_FLOAT);

  Vector2 resolution = { SCREEN_WIDTH, SCREEN_HEIGHT };
  SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uResolution"), &resolution, SHADER_UNIFORM_VEC2);

  int lightsAmount = lights.size();
  SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uLightsAmount"), &lightsAmount, SHADER_UNIFORM_INT);

  int apple = 0;
  #ifdef __APPLE__
  apple = 1;
  #endif
  SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "uApple"), &apple, SHADER_UNIFORM_INT);
}

void Game::render() {
  ClearBackground(PINK);

  BeginShaderMode(lightingShader);
    // <!> SetShaderValueTexture() has to be called while the shader is enabled
    SetShaderValueTexture(lightingShader, GetShaderLocation(lightingShader, "uOcclusionMask"), canvas.tex);
    DrawTexture(canvas.tex, 0, 0, WHITE);
  EndShaderMode();

  for (int i = 0; i < lights.size(); i++) {
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, TextFormat("lights[%i].position", i)), &lights[i].position, SHADER_UNIFORM_VEC2);
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, TextFormat("lights[%i].color",    i)), &lights[i].color,    SHADER_UNIFORM_VEC3);
    SetShaderValue(lightingShader, GetShaderLocation(lightingShader, TextFormat("lights[%i].size",     i)), &lights[i].size,     SHADER_UNIFORM_FLOAT);
    // DrawRectanglePro((Rectangle){ lights[i].position.x, lights[i].position.y, lights[i].size, lights[i].size },
    //                  (Vector2){ lights[i].size/2, lights[i].size/2 },
    //                  0,
    //                  (Color){ lights[i].color.x*255, lights[i].color.y*255, lights[i].color.z*255, 255 });
   }

  // BeginShaderMode(rainbowShader);
  //   DrawRectanglePro((Rectangle){ box.position.x, box.position.y, box.size, box.size },
  //                    (Vector2){ box.size/2, box.size/2 },
  //                    0,
  //                    MAROON);
  // EndShaderMode();

  if (mode == DRAWING)
    DrawTextureEx(brush.tex,
                  (Vector2){ (float)(GetMouseX() - brush.img.width/2*brush.scale),
                             (float)(GetMouseY() - brush.img.height/2*brush.scale) },
                  0.0,
                  brush.scale,
                  BLACK);
}

void Game::renderUI() {
  if (debug) {
    DrawText(TextFormat("%i FPS", GetFPS()), 0, 0, 1, GREEN);
    DrawText(TextFormat("%i lights", lights.size()), 0, 8, 1, GREEN);
  }
}

void Game::processKeyboardInput() {
  if (IsKeyPressed(KEY_F3)) debug = !debug;
  if (IsKeyPressed(KEY_C)) {
    if (mode == DRAWING) {
      std::cout << "Clearing canvas." << std::endl;
      canvas.img = GenImageColor(SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
      UnloadTexture(canvas.tex);
      canvas.tex = LoadTextureFromImage(canvas.img);
    } else if (mode == LIGHTING) {
      std::cout << "Clearing lights." << std::endl;
      lights.clear();
    }
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

  if (IsKeyDown(KEY_ONE)) mode = DRAWING;
  if (IsKeyDown(KEY_TWO)) mode = LIGHTING;
}

void Game::processMouseInput() {
  if (mode == DRAWING) {
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
  } else if (mode == LIGHTING) {
    if (IsMouseButtonPressed(0)) {
      Light l;
      l.position = (Vector2){ static_cast<float>(GetMouseX()), static_cast<float>(GetMouseY()) };
      l.color    = (Vector3){ std::sin(time), std::cos(time), 1.0 };
      l.size     = 600;
      lights.push_back(l);
    } else if (IsMouseButtonDown(1)) {
      for (int i = 0; i < lights.size(); i++) {
        if (Vector2Length(lights[i].position - (Vector2){ static_cast<float>(GetMouseX()), static_cast<float>(GetMouseY()) }) < 16) {
          lights.erase(lights.begin() + i);
        }
      }
    }
  }
}
