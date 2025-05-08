#ifndef PONG_DEMO_H
#define PONG_DEMO_H

#include "demo.h"

struct Paddle {
  Vector2 position;
  bool direction; // up or down
  Vector3 color;
};

class PongDemo: public Demo {
  public:
    PongDemo();
    void processMouseInput() override;
    void processKeyboardInput() override;
    void update() override;
    void resize() override;
    void renderUI() override;
  protected:
    void prepScene() override;

  private:
    void resetBall();

    struct {
      Vector2 position;
      Vector2 direction;
      int size;
      Vector3 color;
      float speed;
    } ball;

    Paddle leftPaddle;
    Paddle rightPaddle;
    Vector2 paddleSize {16, 64};
    float paddleSpeed {128.0};

    #define AI_FRAMERATE .5
    double lastAiFrameTime{};

    WindowData settingsWindowData;
};

#endif /* PONG_DEMO_H */
