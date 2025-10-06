/**
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "Sample.h"
#include "tractor.h"

using namespace tractor;

/**
 * Sample programmatically creating and drawing the supported mesh primitives.
 */
class MeshPrimitiveSample : public Sample
{
  public:
    MeshPrimitiveSample();

    void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);

  protected:
    void initialize();

    void finalize();

    void update(float elapsedTime);

    void render(float elapsedTime);

  private:
    static Material* createMaterial();

    static MeshBatch* createMeshBatch(Mesh::PrimitiveType primitiveType);

    Font* _font;
    Model* _triangles;
    Model* _triangleStrip;
    Model* _lineStrip;
    Model* _lines;
    Model* _points;
    Matrix _viewProjectionMatrix;
    Vector2 _touchPoint;
    Vector2 _tilt;
    std::list<Text*> _text;
};
