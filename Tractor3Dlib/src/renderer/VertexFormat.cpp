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
#include "pch.h"

#include "renderer/VertexFormat.h"

namespace tractor
{

//----------------------------------------------------------------------------
VertexFormat::VertexFormat(const Element* elements, unsigned int elementCount)
{
    assert(elements);

    // Copy elements and compute vertex size
    for (size_t i = 0; i < elementCount; ++i)
    {
        // Copy element
        Element element;
        memcpy(&element, &elements[i], sizeof(Element));
        _elements.push_back(element);

        _vertexSize += element.size * sizeof(float);
    }
}

//----------------------------------------------------------------------------
const VertexFormat::Element& VertexFormat::getElement(unsigned int index) const
{
    assert(index < _elements.size());
    return _elements[index];
}

//----------------------------------------------------------------------------
bool VertexFormat::operator==(const VertexFormat& f) const
{
    if (_elements.size() != f._elements.size()) return false;

    for (size_t i = 0, count = _elements.size(); i < count; ++i)
    {
        if (_elements[i] != f._elements[i]) return false;
    }

    return true;
}

//----------------------------------------------------------------------------
VertexFormat::Element::Element(Usage usage, unsigned int size) : usage(usage), size(size) {}

//----------------------------------------------------------------------------
bool VertexFormat::Element::operator==(const VertexFormat::Element& e) const
{
    return (size == e.size && usage == e.usage);
}

//----------------------------------------------------------------------------
bool VertexFormat::Element::operator!=(const VertexFormat::Element& e) const
{
    return !(*this == e);
}

//----------------------------------------------------------------------------
const char* VertexFormat::toString(Usage usage)
{
    switch (usage)
    {
        case POSITION:
            return "POSITION";
        case NORMAL:
            return "NORMAL";
        case COLOR:
            return "COLOR";
        case TANGENT:
            return "TANGENT";
        case BINORMAL:
            return "BINORMAL";
        case BLENDWEIGHTS:
            return "BLENDWEIGHTS";
        case BLENDINDICES:
            return "BLENDINDICES";
        case TEXCOORD0:
            return "TEXCOORD0";
        case TEXCOORD1:
            return "TEXCOORD1";
        case TEXCOORD2:
            return "TEXCOORD2";
        case TEXCOORD3:
            return "TEXCOORD3";
        case TEXCOORD4:
            return "TEXCOORD4";
        case TEXCOORD5:
            return "TEXCOORD5";
        case TEXCOORD6:
            return "TEXCOORD6";
        case TEXCOORD7:
            return "TEXCOORD7";
        default:
            return "UNKNOWN";
    }
}

} // namespace tractor
