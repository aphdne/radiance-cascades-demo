#ifndef SCENE_H
#define SCENE_H

#include "demo.h"

class Scene {
  public:
    Scene();
    virtual void update();
    virtual void render();

    ImageTexture emissionMap;
    ImageTexture occlusionMap;
};

#endif /* SCENE_H */
