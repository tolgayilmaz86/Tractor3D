#include "pch.h"

#include "renderer/Font.h"

#include "framework/FileSystem.h"
#include "framework/Game.h"
#include "renderer/Material.h"
#include "renderer/Text.h"
#include "scene/Bundle.h"

namespace tractor
{

static std::vector<Font*> __fontCache;

Font::~Font()
{
    // Remove this Font from the font cache.
    std::vector<Font*>::iterator itr = std::find(__fontCache.begin(), __fontCache.end(), this);
    if (itr != __fontCache.end())
    {
        __fontCache.erase(itr);
    }

    SAFE_RELEASE(_texture);

    // Free child fonts
    std::ranges::for_each(_sizes, [](auto* font) { SAFE_RELEASE(font); });
    _sizes.clear();
}

Font* Font::create(const std::string& path, const std::string& id)
{
    // Search the font cache for a font with the given path and ID.
    for (size_t i = 0, count = __fontCache.size(); i < count; ++i)
    {
        Font* f = __fontCache[i];
        assert(f);
        if (f->_path == path && (id.empty() || f->_id == id))
        {
            // Found a match.
            f->addRef();
            return f;
        }
    }

    // Load the bundle.
    Bundle* bundle = Bundle::create(path);
    if (bundle == nullptr)
    {
        GP_WARN("Failed to load font bundle '%s'.", path);
        return nullptr;
    }

    Font* font = nullptr;
    if (id.empty())
    {
        // Get the ID of the first object in the bundle (assume it's a Font).
        std::string id;
        if ((id = bundle->getObjectId(0)) == EMPTY_STRING)
        {
            GP_WARN("Failed to load font without explicit id; the first object in the font bundle "
                    "has a null id.");
            return nullptr;
        }

        // Load the font using the ID of the first object in the bundle.
        font = bundle->loadFont(bundle->getObjectId(0));
    }
    else
    {
        // Load the font with the given ID.
        font = bundle->loadFont(id);
    }

    if (font)
    {
        // Add this font to the cache.
        __fontCache.push_back(font);
    }

    SAFE_RELEASE(bundle);

    return font;
}

Font* Font::create(const std::string& family,
                   Style style,
                   unsigned int size,
                   Glyph* glyphs,
                   int glyphCount,
                   Texture* texture,
                   Font::Format format)
{
    assert(glyphs);
    assert(texture);

    std::string defines{};
    if (format == DISTANCE_FIELD) defines = "DISTANCE_FIELD";

    // Default font shaders
    constexpr auto FONT_VSH = "res/shaders/font.vert";
    constexpr auto FONT_FSH = "res/shaders/font.frag";

    // Create the effect for the font's sprite batch.
    Effect* fontEffect = std::move(Effect::createFromFile(FONT_VSH, FONT_FSH, defines));
    if (fontEffect == nullptr)
    {
        GP_WARN("Failed to create effect for font.");
        SAFE_RELEASE(texture);
        return nullptr;
    }

    // Create batch for the font.
    auto batch = std::unique_ptr<SpriteBatch>(SpriteBatch::create(texture, fontEffect, 128));

    if (batch == nullptr)
    {
        GP_WARN("Failed to create batch for font.");
        return nullptr;
    }

    // Add linear filtering for better font quality.
    Texture::Sampler* sampler = batch->getSampler();
    sampler->setFilterMode(Texture::LINEAR_MIPMAP_LINEAR, Texture::LINEAR);
    sampler->setWrapMode(Texture::CLAMP, Texture::CLAMP);

    // Increase the ref count of the texture to retain it.
    texture->addRef();

    Font* font = new Font();
    font->_format = format;
    font->_family = family;
    font->_style = style;
    font->_size = size;
    font->_texture = texture;
    font->_batch = std::move(batch);

    // Copy the glyphs array.
    font->_glyphs = std::make_unique<Glyph[]>(glyphCount);
    std::memcpy(font->_glyphs.get(), glyphs, sizeof(Glyph) * glyphCount);
    font->_glyphCount = glyphCount;

    return font;
}

unsigned int Font::getSize(unsigned int index) const
{
    assert(index <= _sizes.size());

    // index zero == this font
    return index == 0 ? _size : _sizes[index - 1]->_size;
}

unsigned int Font::getSizeCount() const
{
    return _sizes.size() + 1; // +1 for "this" font
}

bool Font::isCharacterSupported(int character) const
{
    // TODO: Update this once we support unicode fonts
    int glyphIndex = character - 32; // HACK for ASCII
    return (glyphIndex >= 0 && glyphIndex < (int)_glyphCount);
}

void Font::start()
{
    // no-op : fonts now are lazily started on the first draw call
}

void Font::lazyStart()
{
    if (_batch->isStarted()) return; // already started

    // Update the projection matrix for our batch to match the current viewport
    const Rectangle& vp = Game::getInstance()->getViewport();
    if (!vp.isEmpty())
    {
        Game* game = Game::getInstance();
        Matrix projectionMatrix =
            Matrix::createOrthographicOffCenter(vp.x, vp.width, vp.height, vp.y, 0, 1);
        _batch->setProjectionMatrix(projectionMatrix);
    }

    _batch->start();
}

void Font::finish()
{
    // Finish any font batches that have been started
    if (_batch->isStarted()) _batch->finish();

    for (auto& font : _sizes)
        if (font->_batch->isStarted()) font->_batch->finish();
}

Font* Font::findClosestSize(int size)
{
    if (size == (int)_size) return this;

    int diff = abs(size - (int)_size);
    Font* closest = this;
    for (size_t i = 0, count = _sizes.size(); i < count; ++i)
    {
        Font* f = _sizes[i];
        int d = abs(size - (int)f->_size);
        if (d < diff || (d == diff && f->_size > closest->_size)) // prefer scaling down instead of up
        {
            diff = d;
            closest = f;
        }
    }

    return closest;
}

void Font::drawText(const std::string& text,
                    int x,
                    int y,
                    const Vector4& color,
                    unsigned int size,
                    bool rightToLeft)
{
    assert(_size);

    if (size == 0)
    {
        size = _size;
    }
    else
    {
        // Delegate to closest sized font
        Font* f = findClosestSize(size);
        if (f != this)
        {
            f->drawText(text, x, y, color, size, rightToLeft);
            return;
        }
    }

    lazyStart();

    float scale = (float)size / _size;
    int spacing = (int)(size * _spacing);
    const char* cursor = nullptr;

    if (rightToLeft)
    {
        cursor = text.c_str();
    }

    int xPos = x, yPos = y;
    bool done = false;

    while (!done)
    {
        size_t length;
        size_t startIndex;
        int iteration;
        if (rightToLeft)
        {
            char delimiter = cursor[0];
            while (!done
                   && (delimiter == ' ' || delimiter == '\t' || delimiter == '\r'
                       || delimiter == '\n' || delimiter == 0))
            {
                switch (delimiter)
                {
                    case ' ':
                        xPos += _glyphs[0].advance;
                        break;
                    case '\r':
                    case '\n':
                        yPos += size;
                        xPos = x;
                        break;
                    case '\t':
                        xPos += _glyphs[0].advance * 4;
                        break;
                    case 0:
                        done = true;
                        break;
                }

                if (!done)
                {
                    ++cursor;
                    delimiter = cursor[0];
                }
            }

            length = strcspn(cursor, "\r\n");
            startIndex = length - 1;
            iteration = -1;
        }
        else
        {
            length = text.length();
            startIndex = 0;
            iteration = 1;
        }

        assert(_glyphs);
        assert(_batch);
        for (size_t i = startIndex; i < length; i += (size_t)iteration)
        {
            char c = 0;
            if (rightToLeft)
            {
                c = cursor[i];
            }
            else
            {
                c = text[i];
            }

            // Draw this character.
            switch (c)
            {
                case ' ':
                    xPos += _glyphs[0].advance;
                    break;
                case '\r':
                case '\n':
                    yPos += size;
                    xPos = x;
                    break;
                case '\t':
                    xPos += _glyphs[0].advance * 4;
                    break;
                default:
                    int index = c - 32; // HACK for ASCII
                    if (index >= 0 && index < (int)_glyphCount)
                    {
                        Glyph& g = _glyphs.get()[index];

                        if (getFormat() == DISTANCE_FIELD)
                        {
                            if (_cutoffParam == nullptr)
                                _cutoffParam = _batch->getMaterial()->getParameter("u_cutoff");
                            // TODO: Fix me so that smaller font are much smoother
                            _cutoffParam->setVector2(Vector2(1.0, 1.0));
                        }
                        _batch->draw(xPos + (int)(g.bearingX * scale),
                                     yPos,
                                     g.width * scale,
                                     size,
                                     g.uvs[0],
                                     g.uvs[1],
                                     g.uvs[2],
                                     g.uvs[3],
                                     color);
                        xPos += floor(g.advance * scale + spacing);
                        break;
                    }
                    break;
            }
        }

        if (rightToLeft)
            cursor += length;
        else
            done = true;
    }
}

void Font::drawText(const std::string& text,
                    int x,
                    int y,
                    float red,
                    float green,
                    float blue,
                    float alpha,
                    unsigned int size,
                    bool rightToLeft)
{
    drawText(text, x, y, Vector4(red, green, blue, alpha), size, rightToLeft);
}

void Font::drawText(const std::string& text,
                    const Rectangle& area,
                    const Vector4& color,
                    unsigned int size,
                    Justify justify,
                    bool wrap,
                    bool rightToLeft,
                    const Rectangle& clip)
{
    assert(_size);

    if (size == 0)
    {
        size = _size;
    }
    else
    {
        // Delegate to closest sized font
        Font* f = findClosestSize(size);
        if (f != this)
        {
            f->drawText(text, area, color, size, justify, wrap, rightToLeft, clip);
            return;
        }
    }

    lazyStart();

    float scale = (float)size / _size;
    int spacing = (int)(size * _spacing);
    int yPos = area.y;
    const float areaHeight = area.height - size;
    std::vector<int> xPositions;
    std::vector<unsigned int> lineLengths;

    getMeasurementInfo(text, area, size, justify, wrap, rightToLeft, &xPositions, &yPos, &lineLengths);

    // Now we have the info we need in order to render.
    int xPos = area.x;
    std::vector<int>::const_iterator xPositionsIt = xPositions.begin();
    if (xPositionsIt != xPositions.end())
    {
        xPos = *xPositionsIt++;
    }

    const char* token = text.c_str();
    int iteration = 1;
    unsigned int lineLength;
    unsigned int currentLineLength = 0;
    const char* lineStart;
    std::vector<unsigned int>::const_iterator lineLengthsIt;
    if (rightToLeft)
    {
        lineStart = token;
        lineLengthsIt = lineLengths.begin();
        lineLength = *lineLengthsIt++;
        token += lineLength - 1;
        iteration = -1;
    }

    while (token[0] != 0)
    {
        // Handle delimiters until next token.
        if (!handleDelimiters(&token,
                              size,
                              iteration,
                              area.x,
                              &xPos,
                              &yPos,
                              &currentLineLength,
                              &xPositionsIt,
                              xPositions.end()))
        {
            break;
        }

        bool truncated = false;
        unsigned int tokenLength;
        unsigned int tokenWidth;
        unsigned int startIndex;
        if (rightToLeft)
        {
            tokenLength = getReversedTokenLength(token, text);
            currentLineLength += tokenLength;
            token -= (tokenLength - 1);
            tokenWidth = getTokenWidth(token, tokenLength, size, scale);
            iteration = -1;
            startIndex = tokenLength - 1;
        }
        else
        {
            tokenLength = (unsigned int)strcspn(token, " \r\n\t");
            tokenWidth = getTokenWidth(token, tokenLength, size, scale);
            iteration = 1;
            startIndex = 0;
        }

        // Wrap if necessary.
        if (wrap
            && (xPos + (int)tokenWidth > area.x + area.width
                || (rightToLeft && currentLineLength > lineLength)))
        {
            yPos += (int)size;
            currentLineLength = tokenLength;

            if (xPositionsIt != xPositions.end())
            {
                xPos = *xPositionsIt++;
            }
            else
            {
                xPos = area.x;
            }
        }

        bool draw = true;
        if (yPos < static_cast<int>(area.y - size))
        {
            // Skip drawing until line break or wrap.
            draw = false;
        }
        else if (yPos > area.y + areaHeight)
        {
            // Truncate below area's vertical limit.
            break;
        }

        assert(_glyphs);
        assert(_batch);
        for (size_t i = startIndex; i < (int)tokenLength && i >= 0; i += iteration)
        {
            char c = token[i];
            int glyphIndex = c - 32; // HACK for ASCII

            if (glyphIndex >= 0 && glyphIndex < (int)_glyphCount)
            {
                Glyph& g = _glyphs[glyphIndex];

                if (xPos + (int)(g.advance * scale) > area.x + area.width)
                {
                    // Truncate this line and go on to the next one.
                    truncated = true;
                    break;
                }
                else if (xPos >= (int)area.x)
                {
                    // Draw this character.
                    if (draw)
                    {
                        if (getFormat() == DISTANCE_FIELD)
                        {
                            if (_cutoffParam == nullptr)
                                _cutoffParam = _batch->getMaterial()->getParameter("u_cutoff");
                            // TODO: Fix me so that smaller font are much smoother
                            _cutoffParam->setVector2(Vector2(1.0, 1.0));
                        }
                        if (clip != Rectangle(0, 0, 0, 0))
                        {
                            _batch->draw(xPos + (int)(g.bearingX * scale),
                                         yPos,
                                         g.width * scale,
                                         size,
                                         g.uvs[0],
                                         g.uvs[1],
                                         g.uvs[2],
                                         g.uvs[3],
                                         color,
                                         clip);
                        }
                        else
                        {
                            _batch->draw(xPos + (int)(g.bearingX * scale),
                                         yPos,
                                         g.width * scale,
                                         size,
                                         g.uvs[0],
                                         g.uvs[1],
                                         g.uvs[2],
                                         g.uvs[3],
                                         color);
                        }
                    }
                }
                xPos += (int)(g.advance) * scale + spacing;
            }
        }

        if (!truncated)
        {
            if (rightToLeft)
            {
                if (token == lineStart)
                {
                    token += lineLength;

                    // Now handle delimiters going forwards.
                    if (!handleDelimiters(&token,
                                          size,
                                          1,
                                          area.x,
                                          &xPos,
                                          &yPos,
                                          &currentLineLength,
                                          &xPositionsIt,
                                          xPositions.end()))
                    {
                        break;
                    }

                    if (lineLengthsIt != lineLengths.end())
                    {
                        lineLength = *lineLengthsIt++;
                    }
                    lineStart = token;
                    token += lineLength - 1;
                }
                else
                {
                    token--;
                }
            }
            else
            {
                token += tokenLength;
            }
        }
        else
        {
            if (rightToLeft)
            {
                token = lineStart + lineLength;

                if (!handleDelimiters(&token,
                                      size,
                                      1,
                                      area.x,
                                      &xPos,
                                      &yPos,
                                      &currentLineLength,
                                      &xPositionsIt,
                                      xPositions.end()))
                {
                    break;
                }

                if (lineLengthsIt != lineLengths.end())
                {
                    lineLength = *lineLengthsIt++;
                }
                lineStart = token;
                token += lineLength - 1;
            }
            else
            {
                // Skip the rest of this line.
                size_t tokenLength = strcspn(token, "\n");

                if (tokenLength > 0)
                {
                    // Get first token of next line.
                    token += tokenLength;
                }
            }
        }
    }
}

void Font::measureText(const std::string& text,
                       unsigned int size,
                       unsigned int* width,
                       unsigned int* height)
{
    assert(_size);
    assert(width);
    assert(height);

    if (size == 0)
    {
        size = _size;
    }
    else
    {
        // Delegate to closest sized font
        Font* f = findClosestSize(size);
        if (f != this)
        {
            f->measureText(text, size, width, height);
            return;
        }
    }

    const size_t length = text.length();
    if (length == 0)
    {
        *width = 0;
        *height = 0;
        return;
    }

    float scale = (float)size / _size;
    const char* token = text.c_str();

    *width = 0;
    *height = size;

    // Measure a line at a time.
    while (token[0] != 0)
    {
        while (token[0] == '\n')
        {
            *height += size;
            ++token;
        }

        unsigned int tokenLength = (unsigned int)strcspn(token, "\n");
        unsigned int tokenWidth = getTokenWidth(token, tokenLength, size, scale);
        if (tokenWidth > *width)
        {
            *width = tokenWidth;
        }

        token += tokenLength;
    }
}

void Font::measureText(const std::string& text,
                       const Rectangle& clip,
                       unsigned int size,
                       Rectangle* out,
                       Justify justify,
                       bool wrap,
                       bool ignoreClip)
{
    assert(_size);
    assert(out);

    if (size == 0)
    {
        size = _size;
    }
    else
    {
        // Delegate to closest sized font
        Font* f = findClosestSize(size);
        if (f != this)
        {
            f->measureText(text, clip, size, out, justify, wrap, ignoreClip);
            return;
        }
    }

    if (text.empty())
    {
        out->set(0, 0, 0, 0);
        return;
    }

    float scale = (float)size / _size;
    Justify vAlign = static_cast<Justify>(justify & 0xF0);
    if (vAlign == 0)
    {
        vAlign = ALIGN_TOP;
    }

    Justify hAlign = static_cast<Justify>(justify & 0x0F);
    if (hAlign == 0)
    {
        hAlign = ALIGN_LEFT;
    }

    const char* token = text.c_str();
    std::vector<bool> emptyLines;
    std::vector<Vector2> lines;

    unsigned int lineWidth = 0;
    int yPos = clip.y + size;
    const float viewportHeight = clip.height;

    if (wrap)
    {
        unsigned int delimWidth = 0;
        bool reachedEOF = false;
        while (token[0] != 0)
        {
            // Handle delimiters until next token.
            char delimiter = token[0];
            while (delimiter == ' ' || delimiter == '\t' || delimiter == '\r' || delimiter == '\n'
                   || delimiter == 0)
            {
                switch (delimiter)
                {
                    case ' ':
                        delimWidth += _glyphs[0].advance;
                        break;
                    case '\r':
                    case '\n':
                        // Add line-height to vertical cursor.
                        yPos += size;

                        if (lineWidth > 0)
                        {
                            // Determine horizontal position and width.
                            int hWhitespace = clip.width - lineWidth;
                            int xPos = clip.x;
                            if (hAlign == ALIGN_HCENTER)
                            {
                                xPos += hWhitespace / 2;
                            }
                            else if (hAlign == ALIGN_RIGHT)
                            {
                                xPos += hWhitespace;
                            }

                            // Record this line's size.
                            emptyLines.push_back(false);
                            lines.emplace_back(Vector2(xPos, lineWidth));
                        }
                        else
                        {
                            // Record the existence of an empty line.
                            emptyLines.push_back(true);
                            lines.emplace_back(Vector2(FLT_MAX, 0));
                        }

                        lineWidth = 0;
                        delimWidth = 0;
                        break;
                    case '\t':
                        delimWidth += _glyphs[0].advance * 4;
                        break;
                    case 0:
                        reachedEOF = true;
                        break;
                }

                if (reachedEOF)
                {
                    break;
                }

                token++;
                delimiter = token[0];
            }

            if (reachedEOF)
            {
                break;
            }

            // Measure the next token.
            unsigned int tokenLength = (unsigned int)strcspn(token, " \r\n\t");
            unsigned int tokenWidth = getTokenWidth(token, tokenLength, size, scale);

            // Wrap if necessary.
            if (lineWidth + tokenWidth + delimWidth > clip.width)
            {
                // Add line-height to vertical cursor.
                yPos += size;

                // Determine horizontal position and width.
                int hWhitespace = clip.width - lineWidth;
                int xPos = clip.x;
                if (hAlign == ALIGN_HCENTER)
                {
                    xPos += hWhitespace / 2;
                }
                else if (hAlign == ALIGN_RIGHT)
                {
                    xPos += hWhitespace;
                }

                // Record this line's size.
                emptyLines.push_back(false);
                lines.emplace_back(Vector2(xPos, lineWidth));
                lineWidth = 0;
            }
            else
            {
                lineWidth += delimWidth;
            }

            delimWidth = 0;
            lineWidth += tokenWidth;
            token += tokenLength;
        }
    }
    else
    {
        // Measure a whole line at a time.
        int emptyLinesCount = 0;
        while (token[0] != 0)
        {
            // Handle any number of consecutive newlines.
            bool nextLine = true;
            while (token[0] == '\n')
            {
                if (nextLine)
                {
                    // Add line-height to vertical cursor.
                    yPos += size * (emptyLinesCount + 1);
                    nextLine = false;
                    emptyLinesCount = 0;
                    emptyLines.push_back(false);
                }
                else
                {
                    // Record the existence of an empty line.
                    ++emptyLinesCount;
                    emptyLines.push_back(true);
                    lines.emplace_back(Vector2(FLT_MAX, 0));
                }

                token++;
            }

            // Measure the next line.
            unsigned int tokenLength = (unsigned int)strcspn(token, "\n");
            lineWidth = getTokenWidth(token, tokenLength, size, scale);

            // Determine horizontal position and width.
            int xPos = clip.x;
            int hWhitespace = clip.width - lineWidth;
            if (hAlign == ALIGN_HCENTER)
            {
                xPos += hWhitespace / 2;
            }
            else if (hAlign == ALIGN_RIGHT)
            {
                xPos += hWhitespace;
            }

            // Record this line's size.
            lines.emplace_back(Vector2(xPos, lineWidth));

            token += tokenLength;
        }

        yPos += size;
    }

    if (wrap)
    {
        // Record the size of the last line.
        int hWhitespace = clip.width - lineWidth;
        int xPos = clip.x;
        if (hAlign == ALIGN_HCENTER)
        {
            xPos += hWhitespace / 2;
        }
        else if (hAlign == ALIGN_RIGHT)
        {
            xPos += hWhitespace;
        }

        lines.emplace_back(Vector2(xPos, lineWidth));
    }

    int x = INT_MAX;
    int y = clip.y;
    unsigned int width = 0;
    int height = yPos - clip.y;

    // Calculate top of text without clipping.
    int vWhitespace = viewportHeight - height;
    if (vAlign == ALIGN_VCENTER)
    {
        y += vWhitespace / 2;
    }
    else if (vAlign == ALIGN_BOTTOM)
    {
        y += vWhitespace;
    }

    int clippedTop = 0;
    int clippedBottom = 0;
    if (!ignoreClip)
    {
        // Trim rect to fit text that would actually be drawn within the given clip.
        if (y >= clip.y)
        {
            // Text goes off the bottom of the clip.
            clippedBottom = (height - viewportHeight) / size + 1;
            if (clippedBottom > 0)
            {
                // Also need to crop empty lines above non-empty lines that have been clipped.
                size_t emptyIndex = emptyLines.size() - clippedBottom;
                while (emptyIndex < emptyLines.size() && emptyLines[emptyIndex] == true)
                {
                    height -= size;
                    emptyIndex++;
                }

                height -= size * clippedBottom;
            }
            else
            {
                clippedBottom = 0;
            }
        }
        else
        {
            // Text goes above the top of the clip.
            clippedTop = (clip.y - y) / size + 1;
            if (clippedTop < 0)
            {
                clippedTop = 0;
            }

            // Also need to crop empty lines below non-empty lines that have been clipped.
            size_t emptyIndex = clippedTop;
            while (emptyIndex < emptyLines.size() && emptyLines[emptyIndex] == true)
            {
                y += size;
                height -= size;
                emptyIndex++;
            }

            if (vAlign == ALIGN_VCENTER)
            {
                // In this case lines may be clipped off the bottom as well.
                clippedBottom = (height - viewportHeight + vWhitespace / 2 + 0.01) / size + 1;
                if (clippedBottom > 0)
                {
                    emptyIndex = emptyLines.size() - clippedBottom;
                    while (emptyIndex < emptyLines.size() && emptyLines[emptyIndex] == true)
                    {
                        height -= size;
                        emptyIndex++;
                    }

                    height -= size * clippedBottom;
                }
                else
                {
                    clippedBottom = 0;
                }
            }

            y = y + size * clippedTop;
            height = height - size * clippedTop;
        }
    }

    // Determine left-most x coordinate and largest width out of lines that have not been clipped.
    for (size_t i = clippedTop; i < (int)lines.size() - clippedBottom; ++i)
    {
        if (lines[i].x < x)
        {
            x = lines[i].x;
        }
        if (lines[i].y > width)
        {
            width = lines[i].y;
        }
    }

    if (!ignoreClip)
    {
        // Guarantee that the output rect will fit within the clip.
        out->x = (x >= clip.x) ? x : clip.x;
        out->y = (y >= clip.y) ? y : clip.y;
        out->width = (width <= clip.width) ? width : clip.width;
        out->height = (height <= viewportHeight) ? height : viewportHeight;
    }
    else
    {
        out->x = x;
        out->y = y;
        out->width = width;
        out->height = height;
    }
}

void Font::getMeasurementInfo(const std::string& text,
                              const Rectangle& area,
                              unsigned int size,
                              Justify justify,
                              bool wrap,
                              bool rightToLeft,
                              std::vector<int>* xPositions,
                              int* yPosition,
                              std::vector<unsigned int>* lineLengths)
{
    assert(_size);
    assert(yPosition);

    if (size == 0) size = _size;

    float scale = (float)size / _size;

    Justify vAlign = static_cast<Justify>(justify & 0xF0);
    if (vAlign == 0)
    {
        vAlign = ALIGN_TOP;
    }

    Justify hAlign = static_cast<Justify>(justify & 0x0F);
    if (hAlign == 0)
    {
        hAlign = ALIGN_LEFT;
    }

    const char* token = text.c_str();
    const float areaHeight = area.height - size;

    // For alignments other than top-left, need to calculate the y position to begin drawing from
    // and the starting x position of each line.  For right-to-left text, need to determine
    // the number of characters on each line.
    if (vAlign != ALIGN_TOP || hAlign != ALIGN_LEFT || rightToLeft)
    {
        int lineWidth = 0;
        int delimWidth = 0;

        if (wrap)
        {
            // Go a word at a time.
            bool reachedEOF = false;
            unsigned int lineLength = 0;
            while (token[0] != 0)
            {
                unsigned int tokenWidth = 0;

                // Handle delimiters until next token.
                char delimiter = token[0];
                while (delimiter == ' ' || delimiter == '\t' || delimiter == '\r'
                       || delimiter == '\n' || delimiter == 0)
                {
                    switch (delimiter)
                    {
                        case ' ':
                            delimWidth += _glyphs[0].advance;
                            lineLength++;
                            break;
                        case '\r':
                        case '\n':
                            *yPosition += size;

                            if (lineWidth > 0)
                            {
                                addLineInfo(area,
                                            lineWidth,
                                            lineLength,
                                            hAlign,
                                            xPositions,
                                            lineLengths,
                                            rightToLeft);
                            }

                            lineWidth = 0;
                            lineLength = 0;
                            delimWidth = 0;
                            break;
                        case '\t':
                            delimWidth += _glyphs[0].advance * 4;
                            lineLength++;
                            break;
                        case 0:
                            reachedEOF = true;
                            break;
                    }

                    if (reachedEOF)
                    {
                        break;
                    }

                    token++;
                    delimiter = token[0];
                }

                if (reachedEOF || token == nullptr)
                {
                    break;
                }

                unsigned int tokenLength = (unsigned int)strcspn(token, " \r\n\t");
                tokenWidth += getTokenWidth(token, tokenLength, size, scale);

                // Wrap if necessary.
                if (lineWidth + tokenWidth + delimWidth > area.width)
                {
                    *yPosition += size;

                    // Push position of current line.
                    if (lineLength)
                    {
                        addLineInfo(area,
                                    lineWidth,
                                    lineLength - 1,
                                    hAlign,
                                    xPositions,
                                    lineLengths,
                                    rightToLeft);
                    }
                    else
                    {
                        addLineInfo(area,
                                    lineWidth,
                                    tokenLength,
                                    hAlign,
                                    xPositions,
                                    lineLengths,
                                    rightToLeft);
                    }

                    // Move token to the next line.
                    lineWidth = 0;
                    lineLength = 0;
                    delimWidth = 0;
                }
                else
                {
                    lineWidth += delimWidth;
                    delimWidth = 0;
                }

                lineWidth += tokenWidth;
                lineLength += tokenLength;
                token += tokenLength;
            }

            // Final calculation of vertical position.
            int textHeight = *yPosition - area.y;
            int vWhiteSpace = areaHeight - textHeight;
            if (vAlign == ALIGN_VCENTER)
            {
                *yPosition = area.y + vWhiteSpace / 2;
            }
            else if (vAlign == ALIGN_BOTTOM)
            {
                *yPosition = area.y + vWhiteSpace;
            }

            // Calculation of final horizontal position.
            addLineInfo(area, lineWidth, lineLength, hAlign, xPositions, lineLengths, rightToLeft);
        }
        else
        {
            // Go a line at a time.
            while (token[0] != 0)
            {
                char delimiter = token[0];
                while (delimiter == '\n')
                {
                    *yPosition += size;
                    ++token;
                    delimiter = token[0];
                }

                unsigned int tokenLength = (unsigned int)strcspn(token, "\n");
                if (tokenLength == 0)
                {
                    tokenLength = (unsigned int)strlen(token);
                }

                int lineWidth = getTokenWidth(token, tokenLength, size, scale);
                addLineInfo(area, lineWidth, tokenLength, hAlign, xPositions, lineLengths, rightToLeft);

                token += tokenLength;
            }

            int textHeight = *yPosition - area.y;
            int vWhiteSpace = areaHeight - textHeight;
            if (vAlign == ALIGN_VCENTER)
            {
                *yPosition = area.y + vWhiteSpace / 2;
            }
            else if (vAlign == ALIGN_BOTTOM)
            {
                *yPosition = area.y + vWhiteSpace;
            }
        }

        if (vAlign == ALIGN_TOP)
        {
            *yPosition = area.y;
        }
    }
}

float Font::getCharacterSpacing() const { return _spacing; }

void Font::setCharacterSpacing(float spacing) { _spacing = spacing; }

int Font::getIndexAtLocation(const std::string& text,
                             const Rectangle& area,
                             unsigned int size,
                             const Vector2& inLocation,
                             Vector2* outLocation,
                             Justify justify,
                             bool wrap,
                             bool rightToLeft)
{
    return getIndexOrLocation(text, area, size, inLocation, outLocation, -1, justify, wrap, rightToLeft);
}

void Font::getLocationAtIndex(const std::string& text,
                              const Rectangle& clip,
                              unsigned int size,
                              Vector2* outLocation,
                              const unsigned int destIndex,
                              Justify justify,
                              bool wrap,
                              bool rightToLeft)
{
    getIndexOrLocation(text,
                       clip,
                       size,
                       *outLocation,
                       outLocation,
                       (const int)destIndex,
                       justify,
                       wrap,
                       rightToLeft);
}

int Font::getIndexOrLocation(const std::string& text,
                             const Rectangle& area,
                             unsigned int size,
                             const Vector2& inLocation,
                             Vector2* outLocation,
                             const int destIndex,
                             Justify justify,
                             bool wrap,
                             bool rightToLeft)
{
    assert(_size);
    assert(outLocation);

    if (size == 0)
    {
        size = _size;
    }
    else
    {
        // Delegate to closest sized font
        Font* f = findClosestSize(size);
        if (f != this)
        {
            return f->getIndexOrLocation(text,
                                         area,
                                         size,
                                         inLocation,
                                         outLocation,
                                         destIndex,
                                         justify,
                                         wrap,
                                         rightToLeft);
        }
    }

    unsigned int charIndex = 0;

    // Essentially need to measure text until we reach inLocation.
    float scale = (float)size / _size;
    int spacing = (int)(size * _spacing);
    int yPos = area.y;
    const float areaHeight = area.height - size;
    std::vector<int> xPositions;
    std::vector<unsigned int> lineLengths;

    getMeasurementInfo(text, area, size, justify, wrap, rightToLeft, &xPositions, &yPos, &lineLengths);

    int xPos = area.x;
    std::vector<int>::const_iterator xPositionsIt = xPositions.begin();
    if (xPositionsIt != xPositions.end())
    {
        xPos = *xPositionsIt++;
    }

    const char* token = text.c_str();

    int iteration = 1;
    unsigned int lineLength;
    unsigned int currentLineLength = 0;
    const char* lineStart;
    std::vector<unsigned int>::const_iterator lineLengthsIt;
    if (rightToLeft)
    {
        lineStart = token;
        lineLengthsIt = lineLengths.begin();
        lineLength = *lineLengthsIt++;
        token += lineLength - 1;
        iteration = -1;
    }

    while (token[0] != 0)
    {
        // Handle delimiters until next token.
        unsigned int delimLength = 0;
        int result;
        if (destIndex == -1)
        {
            result = handleDelimiters(&token,
                                      size,
                                      iteration,
                                      area.x,
                                      &xPos,
                                      &yPos,
                                      &delimLength,
                                      &xPositionsIt,
                                      xPositions.end(),
                                      &charIndex,
                                      &inLocation);
        }
        else
        {
            result = handleDelimiters(&token,
                                      size,
                                      iteration,
                                      area.x,
                                      &xPos,
                                      &yPos,
                                      &delimLength,
                                      &xPositionsIt,
                                      xPositions.end(),
                                      &charIndex,
                                      nullptr,
                                      charIndex,
                                      destIndex);
        }

        currentLineLength += delimLength;
        if (result == 0 || result == 2)
        {
            outLocation->x = xPos;
            outLocation->y = yPos;
            return charIndex;
        }

        if (destIndex == (int)charIndex
            || (destIndex == -1 && inLocation.x >= xPos && inLocation.x < xPos + spacing
                && inLocation.y >= yPos && inLocation.y < yPos + size))
        {
            outLocation->x = xPos;
            outLocation->y = yPos;
            return charIndex;
        }

        bool truncated = false;
        unsigned int tokenLength;
        unsigned int tokenWidth;
        unsigned int startIndex;
        if (rightToLeft)
        {
            tokenLength = getReversedTokenLength(token, text);
            currentLineLength += tokenLength;
            charIndex += tokenLength;
            token -= (tokenLength - 1);
            tokenWidth = getTokenWidth(token, tokenLength, size, scale);
            iteration = -1;
            startIndex = tokenLength - 1;
        }
        else
        {
            tokenLength = (unsigned int)strcspn(token, " \r\n\t");
            tokenWidth = getTokenWidth(token, tokenLength, size, scale);
            iteration = 1;
            startIndex = 0;
        }

        // Wrap if necessary.
        if (wrap
            && (xPos + (int)tokenWidth > area.x + area.width
                || (rightToLeft && currentLineLength > lineLength)))
        {
            yPos += size;
            currentLineLength = tokenLength;

            if (xPositionsIt != xPositions.end())
            {
                xPos = *xPositionsIt++;
            }
            else
            {
                xPos = area.x;
            }
        }

        if (yPos > area.y + areaHeight)
        {
            // Truncate below area's vertical limit.
            break;
        }

        assert(_glyphs);
        for (size_t i = startIndex; i < (int)tokenLength && i >= 0; i += iteration)
        {
            char c = token[i];
            int glyphIndex = c - 32; // HACK for ASCII

            if (glyphIndex >= 0 && glyphIndex < (int)_glyphCount)
            {
                Glyph& g = _glyphs[glyphIndex];

                if (xPos + (int)(g.advance * scale) > area.x + area.width)
                {
                    // Truncate this line and go on to the next one.
                    truncated = true;
                    break;
                }

                // Check against inLocation.
                //  Note: g.width is smaller than g.advance, so if I only check against g.width, I
                //  will miss locations towards the right of the character.
                if (destIndex == (int)charIndex
                    || (destIndex == -1 && inLocation.x >= xPos
                        && inLocation.x < floor(xPos + g.advance * scale + spacing)
                        && inLocation.y >= yPos && inLocation.y < yPos + size))
                {
                    outLocation->x = xPos;
                    outLocation->y = yPos;
                    return charIndex;
                }

                xPos += floor(g.advance * scale + spacing);
                charIndex++;
            }
        }

        if (!truncated)
        {
            if (rightToLeft)
            {
                if (token == lineStart)
                {
                    token += lineLength;

                    // Now handle delimiters going forwards.
                    if (!handleDelimiters(&token,
                                          size,
                                          1,
                                          area.x,
                                          &xPos,
                                          &yPos,
                                          &currentLineLength,
                                          &xPositionsIt,
                                          xPositions.end()))
                    {
                        break;
                    }
                    charIndex += currentLineLength;

                    if (lineLengthsIt != lineLengths.end())
                    {
                        lineLength = *lineLengthsIt++;
                    }
                    lineStart = token;
                    token += lineLength - 1;
                    charIndex += tokenLength;
                }
                else
                {
                    token--;
                }
            }
            else
            {
                token += tokenLength;
            }
        }
        else
        {
            if (rightToLeft)
            {
                token = lineStart + lineLength;

                if (!handleDelimiters(&token,
                                      size,
                                      1,
                                      area.x,
                                      &xPos,
                                      &yPos,
                                      &currentLineLength,
                                      &xPositionsIt,
                                      xPositions.end()))
                {
                    break;
                }

                if (lineLengthsIt != lineLengths.end())
                {
                    lineLength = *lineLengthsIt++;
                }
                lineStart = token;
                token += lineLength - 1;
            }
            else
            {
                // Skip the rest of this line.
                unsigned int tokenLength = (unsigned int)strcspn(token, "\n");

                if (tokenLength > 0)
                {
                    // Get first token of next line.
                    token += tokenLength;
                    charIndex += tokenLength;
                }
            }
        }
    }

    if (destIndex == (int)charIndex
        || (destIndex == -1 && inLocation.x >= xPos && inLocation.x < xPos + spacing
            && inLocation.y >= yPos && inLocation.y < yPos + size))
    {
        outLocation->x = xPos;
        outLocation->y = yPos;
        return charIndex;
    }

    return -1;
}

unsigned int Font::getTokenWidth(const char* token, unsigned int length, unsigned int size, float scale)
{
    assert(_glyphs);

    if (size == 0) size = _size;

    int spacing = (int)(size * _spacing);

    // Calculate width of word or line.
    unsigned int tokenWidth = 0;
    for (size_t i = 0; i < length; ++i)
    {
        char c = token[i];
        switch (c)
        {
            case ' ':
                tokenWidth += _glyphs[0].advance;
                break;
            case '\t':
                tokenWidth += _glyphs[0].advance * 4;
                break;
            default:
                int glyphIndex = c - 32;
                if (glyphIndex >= 0 && glyphIndex < (int)_glyphCount)
                {
                    Glyph& g = _glyphs[glyphIndex];
                    tokenWidth += floor(g.advance * scale + spacing);
                }
                break;
        }
    }

    return tokenWidth;
}

unsigned int Font::getReversedTokenLength(const std::string& token, const std::string& bufStart)
{
    const char* cursor = token.c_str();
    char c = cursor[0];
    unsigned int length = 0;

    while (cursor != bufStart && c != ' ' && c != '\r' && c != '\n' && c != '\t')
    {
        length++;
        cursor--;
        c = cursor[0];
    }

    if (cursor == bufStart)
    {
        length++;
    }

    return length;
}

int Font::handleDelimiters(const char** token,
                           const unsigned int size,
                           const int iteration,
                           const int areaX,
                           int* xPos,
                           int* yPos,
                           unsigned int* lineLength,
                           std::vector<int>::const_iterator* xPositionsIt,
                           std::vector<int>::const_iterator xPositionsEnd,
                           unsigned int* charIndex,
                           const Vector2* stopAtPosition,
                           const int currentIndex,
                           const int destIndex)
{
    assert(token);
    assert(*token);
    assert(xPos);
    assert(yPos);
    assert(lineLength);
    assert(xPositionsIt);

    char delimiter = *token[0];
    bool nextLine = true;
    while (delimiter == ' ' || delimiter == '\t' || delimiter == '\r' || delimiter == '\n'
           || delimiter == 0)
    {
        if ((stopAtPosition && stopAtPosition->x >= *xPos
             && stopAtPosition->x < *xPos + ((int)size >> 1) && stopAtPosition->y >= *yPos
             && stopAtPosition->y < *yPos + (int)size)
            || (currentIndex >= 0 && destIndex >= 0 && currentIndex + (int)*lineLength == destIndex))
        {
            // Success + stopAtPosition was reached.
            return 2;
        }

        switch (delimiter)
        {
            case ' ':
                *xPos += _glyphs[0].advance;
                (*lineLength)++;
                if (charIndex)
                {
                    (*charIndex)++;
                }
                break;
            case '\r':
            case '\n':
                *yPos += size;

                // Only use next xPos for first newline character (in case of multiple consecutive newlines).
                if (nextLine)
                {
                    if (*xPositionsIt != xPositionsEnd)
                    {
                        *xPos = **xPositionsIt;
                        (*xPositionsIt)++;
                    }
                    else
                    {
                        *xPos = areaX;
                    }
                    nextLine = false;
                    *lineLength = 0;
                    if (charIndex)
                    {
                        (*charIndex)++;
                    }
                }
                break;
            case '\t':
                *xPos += _glyphs[0].advance * 4;
                (*lineLength)++;
                if (charIndex)
                {
                    (*charIndex)++;
                }
                break;
            case 0:
                // EOF reached.
                return 0;
        }

        *token += iteration;
        delimiter = *token[0];
    }

    // Success.
    return 1;
}

void Font::addLineInfo(const Rectangle& area,
                       int lineWidth,
                       int lineLength,
                       Justify hAlign,
                       std::vector<int>* xPositions,
                       std::vector<unsigned int>* lineLengths,
                       bool rightToLeft)
{
    int hWhitespace = area.width - lineWidth;
    if (hAlign == ALIGN_HCENTER)
    {
        assert(xPositions);
        (*xPositions).push_back(area.x + hWhitespace / 2);
    }
    else if (hAlign == ALIGN_RIGHT)
    {
        assert(xPositions);
        (*xPositions).push_back(area.x + hWhitespace);
    }

    if (rightToLeft)
    {
        assert(lineLengths);
        (*lineLengths).push_back(lineLength);
    }
}

SpriteBatch* Font::getSpriteBatch(unsigned int size) const
{
    if (size == 0) return _batch.get();

    // Find the closest sized child font
    return const_cast<Font*>(this)->findClosestSize(size)->_batch.get();
}

Font::Justify Font::getJustify(const std::string& justify)
{
    if (justify.empty())
    {
        return Font::ALIGN_TOP_LEFT;
    }

    // Static lookup table for case-insensitive string to enum mapping
    static const std::unordered_map<std::string, Font::Justify> justifyMap = {
        { "align_left", Font::ALIGN_LEFT },
        { "align_hcenter", Font::ALIGN_HCENTER },
        { "align_right", Font::ALIGN_RIGHT },
        { "align_top", Font::ALIGN_TOP },
        { "align_vcenter", Font::ALIGN_VCENTER },
        { "align_bottom", Font::ALIGN_BOTTOM },
        { "align_top_left", Font::ALIGN_TOP_LEFT },
        { "align_vcenter_left", Font::ALIGN_VCENTER_LEFT },
        { "align_bottom_left", Font::ALIGN_BOTTOM_LEFT },
        { "align_top_hcenter", Font::ALIGN_TOP_HCENTER },
        { "align_vcenter_hcenter", Font::ALIGN_VCENTER_HCENTER },
        { "align_bottom_hcenter", Font::ALIGN_BOTTOM_HCENTER },
        { "align_top_right", Font::ALIGN_TOP_RIGHT },
        { "align_vcenter_right", Font::ALIGN_VCENTER_RIGHT },
        { "align_bottom_right", Font::ALIGN_BOTTOM_RIGHT }
    };

    // Convert input to lowercase for case-insensitive comparison
    std::string lowerJustify = justify;
    std::transform(lowerJustify.begin(),
                   lowerJustify.end(),
                   lowerJustify.begin(),
                   [](auto c) { return std::tolower(c); });

    // Look up the value in the map
    auto it = justifyMap.find(lowerJustify);
    if (it != justifyMap.end())
    {
        return it->second;
    }

    // Default case - log warning and return default value
    GP_WARN("Invalid alignment string: '%s'. Defaulting to ALIGN_TOP_LEFT.", justify.c_str());
    return Font::ALIGN_TOP_LEFT;
}

} // namespace tractor
