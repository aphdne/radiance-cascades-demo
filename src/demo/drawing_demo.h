#ifndef DRAWING_DEMO_H
#define DRAWING_DEMO_H

#include "demo.h"

enum Mode {
  DRAWING,
  LIGHTING,
};

class DrawingDemo: public Demo {
  public:
    DrawingDemo();
    void renderUI();
    void processKeyboardInput() override;
    void processMouseInput() override;
    void resize() override;

  private:
    void prepScene() override;
    void setScene(int scene);

    struct {
      Mode    mode;
      Texture brushTexture;
      float   brushSize;
      Color   brushColor;
      void setRandomColor() {
        brushColor = ColorFromNormalized(
                       Vector4{
                         (std::sin(static_cast<float>(GetTime()))   + 1) / 2,
                         (std::cos(static_cast<float>(GetTime()))   + 1) / 2,
                         (std::sin(static_cast<float>(GetTime())*2) + 1) / 2,
                         1.0
                       }
                     );
        }
    } user;

    Vector2 lastMousePos;
    unsigned short framesSinceLastMousePos;
    WindowData infoWindowData;
    WindowData colorWindowData;
    WindowData settingsWindowData;
    float timeSinceScreenshot;
    int selectedScene;

    const Texture2D UI_0{LoadTexture("res/textures/ui/0_trad16rays.png")};
    const Texture2D UI_1{LoadTexture("res/textures/ui/1_trad4rays.png")};
    const Texture2D UI_2{LoadTexture("res/textures/ui/2_trad4raystoofar.png")};
    const Texture2D UI_3{LoadTexture("res/textures/ui/3_trad16rays.png")};
    const Texture2D UI_4{LoadTexture("res/textures/ui/4_radianceinterval.png")};
    const Texture2D UI_5A{LoadTexture("res/textures/ui/5a_penumbra.png")};
    const Texture2D UI_5{LoadTexture("res/textures/ui/5_bilinear.png")};
    const Texture2D UI_6{LoadTexture("res/textures/ui/6_cascade0.png")};
    const Texture2D UI_7{LoadTexture("res/textures/ui/7_cascade1.png")};

    const Texture2D cursorTex{LoadTexture("res/textures/cursor.png")};
};

#endif /* DRAWING_DEMO_H */
