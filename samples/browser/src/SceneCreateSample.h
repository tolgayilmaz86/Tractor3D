#pragma once

#include "tractor.h"
#include "Sample.h"

using namespace tractor;

/**
 * Samples programattically contructing a scene.
 */
class SceneCreateSample : public Sample
{
public:

  SceneCreateSample();

  void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);

protected:

  void initialize();

  void finalize();

  void update(float elapsedTime);

  void render(float elapsedTime);

private:

  bool drawScene(Node* node);

  Font* _font;
  Scene* _scene;
  Node* _cubeNode;
};
