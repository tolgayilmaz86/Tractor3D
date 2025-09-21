#pragma once

#include "tractor.h"
#include "Sample.h"

using namespace tractor;

/**
 * Sample creating and draw a single triangle.
 */
class TriangleSample : public Sample
{
public:

  TriangleSample();

  void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);

protected:

  void initialize();

  void finalize();

  void update(float elapsedTime);

  void render(float elapsedTime);

private:

  Font* _font;
  Model* _model;
  float _spinDirection;
  Matrix _worldViewProjectionMatrix;
};
