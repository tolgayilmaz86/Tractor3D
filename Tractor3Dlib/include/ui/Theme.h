#pragma once

#include "pch.h"

#include "graphics/Rectangle.h"
#include "renderer/Font.h"
#include "renderer/Texture.h"
#include "scene/Properties.h"
#include "utils/Ref.h"

namespace tractor
{

/**
 * Defines a theme used to represent the look or appearance of controls.
 *
 * Once loaded, the appearance properties can be retrieved from their style IDs and set on other
 * UI controls.  A Theme has one property, 'texture', which points to a texture atlas containing
 * all the images used by the theme.  Cursor images, skins, and lists of images used by controls
 * are defined in their own namespaces.  The rest of the Theme consists of Style namespaces.
 * A Style describes the border, margin, and padding of a Control, what images, skins, and cursors
 * are associated with a Control, and Font properties to apply to a Control's text.
 */
class Theme : public Ref
{
    friend class Control;
    friend class Form;
    friend class Skin;
    friend class Game;

  public:
    /**
     * Class representing a set of themed attributes that can be
     * assigned to a control.
     *
     * Defined in "ThemeStyle.h"
     */
    class Style;
    friend class Style;

    /**
     * Struct representing the UV coordinates of a rectangular image.
     */
    struct UVs
    {
        /**
         * Constructor.
         */
        UVs();

        /**
         * Constructor
         *
         * @param u1 u component of the first UV coordinate.
         * @param v1 v component of the first UV coordinate.
         * @param u2 u component of the second UV coordinate.
         * @param v2 v component of the second UV coordinate.
         */
        UVs(float u1, float v1, float u2, float v2);

        /**
         * Gets an empty UVs struct (0, 0, 0, 0).
         */
        static const UVs& empty();

        /**
         * Gets UVs covering an entire texture (0, 0, 1, 1).
         */
        static const UVs& full();

        /**
         * u component of the first UV coordinate.
         */
        float u1;

        /**
         * v component of the first UV coordinate.
         */
        float v1;

        /**
         * u component of the second UV coordinate.
         */
        float u2;

        /**
         * v component of the second UV coordinate.
         */
        float v2;
    };

    /**
     * Struct representing margin, border, and padding areas by
     * the width or height of each side.
     */
    struct SideRegions
    {
        /**
         * Constructor.
         */
        SideRegions() : top(0), bottom(0), left(0), right(0) {}

        /**
         * Gets an empty SideRegion.
         */
        static const SideRegions& empty();

        /**
         * The top of the SideRegion.
         */
        float top;

        /**
         * The bottom of the SideRegion.
         */
        float bottom;

        /**
         * The left side of the SideRegion.
         */
        float left;

        /**
         * The right side of the SideRegion.
         */
        float right;
    };

    /**
     * Struct representing margin areas by the width or height of each side.
     */
    using Margin = SideRegions;

    /**
     * Struct representing border areas by the width or height of each side.
     */
    using Border = SideRegions;

    /**
     * Struct representing padding areas by the width or height of each side.
     */
    using Padding = SideRegions;

    /**
     * Class representing an image within the theme's texture atlas.
     * An image has a region and a blend color in addition to an ID.
     * UV coordinates are calculated from the region and can be retrieved.
     */
    class ThemeImage : public Ref
    {
        friend class Theme;
        friend class Control;

      public:
        /**
         * Gets the ID of the ThemeImage.
         */
        const std::string& getId() const noexcept { return _id; }

        /**
         * Gets the UV coordinates for the ThemeImage.
         */
        const Theme::UVs& getUVs() const noexcept { return _uvs; }
        
        /**
         * Gets the Rectangle region of the ThemeImage.
         */
        const Rectangle& getRegion() const noexcept { return _region; }

        /**
         * Gets the color of the ThemeImage in a Vector4.
         */
        const Vector4& getColor() const noexcept { return _color; }

      private:
        ThemeImage(float tw, float th, const Rectangle& region, const Vector4& color);

        ~ThemeImage();

        static ThemeImage* create(float tw,
                                  float th,
                                  Properties* properties,
                                  const Vector4& defaultColor);

        std::string _id;
        UVs _uvs;
        Rectangle _region;
        Vector4 _color;
    };

    /**
     * Creates a theme using the data from the Properties object defined at the specified URL,
     * where the URL is of the format "<file-path>.<extension>#<namespace-id>/<namespace-id>/.../<namespace-id>"
     * (and "#<namespace-id>/<namespace-id>/.../<namespace-id>" is optional).
     *
     * @param url The URL pointing to the Properties object defining the theme.
     * @script{create}
     */
    static Theme* create(const std::string& url);

    /**
     * Returns the default theme.
     *
     * @return The default theme.
     */
    static Theme* getDefault();

    /**
     * Get a style by its ID.
     *
     * @param id The style ID.
     *
     * @return The style with the specified ID, or nullptr if it does not exist.
     */
    Theme::Style* getStyle(const std::string& id) const;

    /**
     * Get the empty style.  Used when a control does not specify a style.
     * This is especially useful for containers that are being used only for
     * layout and positioning, and have no background or border themselves.
     * The empty style has no border, background, margin, padding, images, etc..
     * Any needed properties can be set on the control directly.
     *
     * @return The empty style.
     */
    Theme::Style* getEmptyStyle();

    /**
     * Returns the sprite batch for this theme.
     *
     * @return The theme's sprite batch.
     */
    SpriteBatch* getSpriteBatch() const noexcept { return _spriteBatch; }

  private:
    /**
     * Class representing a collection of theme images.  An image list
     * can be assigned to each overlay of a style, and controls
     * using the style can then retrieve images by ID in order to draw themselves.
     */
    class ImageList : public Ref
    {
        friend class Theme;
        friend class Control;

      public:
        const std::string& getId() const noexcept { return _id; }

        ThemeImage* getImage(const std::string& imageId) const;

      private:
        ImageList(const Vector4& color);

        ImageList(const ImageList& copy);

        ~ImageList();

        /**
         * Hidden copy assignment operator.
         */
        ImageList& operator=(const ImageList&) = delete;

        static ImageList* create(float tw, float th, Properties* properties);

        std::string _id;
        std::vector<ThemeImage*> _images;
        Vector4 _color;
    };

    /**
     * A skin defines the border and background of a control.
     */
    class Skin : public Ref
    {
        friend class Theme;

      public:
        enum SkinArea
        {
            TOP_LEFT,
            TOP,
            TOP_RIGHT,
            LEFT,
            CENTER,
            RIGHT,
            BOTTOM_LEFT,
            BOTTOM,
            BOTTOM_RIGHT
        };

        /**
         * Gets this skin's ID.
         *
         * @return This skin's ID.
         */
        const std::string& getId() const noexcept { return _id; }

        /**
         * Gets this skin's border.
         *
         * @return This skin's border.
         */
        const Theme::Border& getBorder() const noexcept { return _border; }

        /**
         * Gets the skin region within the theme texture.
         *
         * @return The skin region.
         */
        const Rectangle& getRegion() const noexcept { return _region; }

        /**
         * Gets this skin's UVs.
         *
         * @return This skin's UVs.
         */
        const Theme::UVs& getUVs(SkinArea area) const { return _uvs[area]; }

        /**
         * Gets this skin's color.
         *
         * @return This skin's color.
         */
        const Vector4& getColor() const noexcept { return _color; }

      private:
        Skin(float tw,
             float th,
             const Rectangle& region,
             const Theme::Border& border,
             const Vector4& color);

        ~Skin();

        /**
         * Hidden copy assignment operator.
         */
        Skin& operator=(const Skin&) = delete;

        static Skin* create(const std::string& id,
                            float tw,
                            float th,
                            const Rectangle& region,
                            const Theme::Border& border,
                            const Vector4& color);

        void setRegion(const Rectangle& region, float tw, float th);

        std::string _id;
        Theme::Border _border;
        UVs _uvs[9];
        Vector4 _color;
        Rectangle _region;
        float _tw, _th;
    };

    /**
     * Constructor.
     */
    Theme() = default;

    /**
     * Constructor.
     */
    Theme(const Theme& theme);

    /**
     * Destructor.
     */
    ~Theme();

    /**
     * Cleans up any theme related resources when the game shuts down.
     */
    static void finalize();

    /**
     * Hidden copy assignment operator.
     */
    Theme& operator=(const Theme&) = delete;

    void setProjectionMatrix(const Matrix& matrix);

    static void generateUVs(float tw, float th, float x, float y, float width, float height, UVs* uvs);

    void lookUpSprites(const Properties* overlaySpace,
                       ImageList** imageList,
                       ThemeImage** mouseCursor,
                       Skin** skin);

    std::string _url;
    Texture* _texture{ nullptr };
    SpriteBatch* _spriteBatch{ nullptr };
    Theme::ThemeImage* _emptyImage{ nullptr };
    std::vector<Style*> _styles{};
    std::vector<ThemeImage*> _images{};
    std::vector<ImageList*> _imageLists{};
    std::vector<Skin*> _skins{};
    std::set<Font*> _fonts{};
};

} // namespace tractor
