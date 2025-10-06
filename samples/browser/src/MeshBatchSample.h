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
 * Sample drawing static mesh geometry using MeshBatch.
 */
class MeshBatchSample : public Sample
{
  public:
    MeshBatchSample();

    void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);

  protected:
    void initialize();

    void finalize();

    void update(float elapsedTime);

    void render(float elapsedTime);

  private:
    void addTriangle(int x, int y);

    struct Vertex
    {
        Vector3 position;
        Vector3 color;

        Vertex() {}

        Vertex(const Vector3& position, const Vector3& color) : position(position), color(color) {}
    };

    Font* _font;
    MeshBatch* _meshBatch;
    Matrix _worldViewProjectionMatrix;
    std::vector<Vertex> _vertices;
    double _lastTriangleAdded;
};
