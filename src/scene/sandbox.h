#ifndef SANDBOX_H
#define SANDBOX_H

#include "scene.h"

class Sandbox : Scene {
  public:
    Scene();
    void update() override;
    void render() override;
};

#endif /* SANDBOX_H */
