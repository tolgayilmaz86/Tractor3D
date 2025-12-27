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
#include <vector>

using namespace tractor;

/**
 * Sample post processing.
 */
class PostProcessSample : public Sample
{
  public:
    /**
     * Constructor.
     */
    PostProcessSample() = default;

    /**
     * @see Sample::touchEvent
     */
    void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);

    /**
     * Compositing blitter with a specified material/technique applied from a source buffer into the
     * destination buffer.
     *
     * If destination buffer is nullptr then it composites to the default frame buffer.
     *
     * Requried uniforms:
     * sampler2d u_texture - The input texture sampler
     */
    class Compositor
    {
      public:
        static Compositor* create(FrameBufferPtr srcBuffer,
                                  FrameBufferPtr dstBuffer,
                                  const std::string& materialPath,
                                  const std::string& techniqueId);

        ~Compositor();

        FrameBufferPtr getSrcFrameBuffer() const;

        FrameBufferPtr getDstFrameBuffer() const;

        const std::string& getTechniqueId() const;

        Material* getMaterial() const;

        void blit(const Rectangle& dst);

      private:
        Compositor();

        Compositor(FrameBufferPtr srcBuffer,
                   FrameBufferPtr dstBuffer,
                   MaterialPtr material,
                   const std::string& techniqueId);

        FrameBufferPtr _srcBuffer;
        FrameBufferPtr _dstBuffer;
        MaterialPtr _material;
        const std::string _techniqueId;
    };

  protected:
    void initialize();

    void finalize();

    void update(float elapsedTime);

    void render(float elapsedTime);

  private:
    bool drawScene(Node* node);

    void drawTechniqueId(const std::string& techniqueId);

  private:
    FontPtr _font{nullptr};
    ScenePtr _scene;
    Node* _modelNode{nullptr};
    FrameBufferPtr _frameBuffer;
    unsigned int _compositorIndex{0};
    std::vector<Compositor*> _compositors;
    static ModelPtr _quadModel;
    static MaterialPtr _compositorMaterial;
};
