////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2017 Laurent Gomila (laurent@sfml-dev.org)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <cmath>
#include "cross2d/c2d.h"
#include "cross2d/skeleton/sfml/Utf.hpp"
#include <codecvt>
#include <locale>

using namespace c2d;

namespace {

    // Add an underline or strikethrough line to the vertex array
    void addLine(c2d::VertexArray &vertices, c2d::Vector2i texSize,
                 float lineLength, float lineTop, const c2d::Color &color, float offset,
                 float thickness, float outlineThickness = 0) {
        float top = std::floor(lineTop + offset - (thickness / 2) + 0.5f);
        float bottom = top + std::floor(thickness + 0.5f);

        Vector2f texCoords = {1.0f / (float) texSize.x, 1.0f / (float) texSize.y};

        vertices.append(Vertex({-outlineThickness, top - outlineThickness}, color, texCoords));
        vertices.append(Vertex({lineLength + outlineThickness, top - outlineThickness}, color, texCoords));
        vertices.append(Vertex({-outlineThickness, bottom + outlineThickness}, color, texCoords));
        vertices.append(Vertex({-outlineThickness, bottom + outlineThickness}, color, texCoords));
        vertices.append(Vertex({lineLength + outlineThickness, top - outlineThickness}, color, texCoords));
        vertices.append(Vertex({lineLength + outlineThickness, bottom + outlineThickness}, color, texCoords));
    }

    // Add a glyph quad to the vertex array
    void addGlyphQuad(c2d::VertexArray &vertices, c2d::Vector2i texSize, c2d::Vector2f position,
                      const c2d::Color &color, const c2d::Glyph &glyph, float italic) {
        float padding = 1.0f;

        float left = glyph.bounds.left - padding;
        float top = glyph.bounds.top - padding;
        float right = glyph.bounds.left + glyph.bounds.width + padding;
        float bottom = glyph.bounds.top + glyph.bounds.height + padding;

        float u1 = ((float) glyph.textureRect.left - padding) / (float) texSize.x;
        float v1 = ((float) glyph.textureRect.top - padding) / (float) texSize.y;
        float u2 = ((float) (glyph.textureRect.left + glyph.textureRect.width) + padding) / (float) texSize.x;
        float v2 = ((float) (glyph.textureRect.top + glyph.textureRect.height) + padding) / (float) texSize.y;

        vertices.append(Vertex({position.x + left - italic * top, position.y + top}, color, {u1, v1}));
        vertices.append(Vertex({position.x + right - italic * top, position.y + top}, color, {u2, v1}));
        vertices.append(Vertex({position.x + left - italic * bottom, position.y + bottom}, color, {u1, v2}));
        vertices.append(Vertex({position.x + left - italic * bottom, position.y + bottom}, color, {u1, v2}));
        vertices.append(Vertex({position.x + right - italic * top, position.y + top}, color, {u2, v1}));
        vertices.append(Vertex({position.x + right - italic * bottom, position.y + bottom}, color, {u2, v2}));
    }
}


namespace c2d {

////////////////////////////////////////////////////////////
    Text::Text() :
            m_string(),
            m_font(c2d_renderer->getFont()),
            m_characterSize(C2D_DEFAULT_CHAR_SIZE),
            m_style(Regular),
            m_overflow(Clamp),
            m_fillColor(255, 255, 255),
            m_outlineColor(0, 0, 0),
            m_outlineThickness(0),
            m_vertices(Triangles),
            m_outlineVertices(Triangles),
            m_bounds(),
            m_geometryNeedUpdate(false) {
        type = Type::Text;
    }

////////////////////////////////////////////////////////////
    Text::Text(const std::string &string, unsigned int characterSize, Font *font) :
            m_string(string),
            m_font(font),
            m_characterSize(characterSize),
            m_style(Regular),
            m_overflow(Clamp),
            m_fillColor(255, 255, 255),
            m_outlineColor(0, 0, 0),
            m_outlineThickness(0),
            m_vertices(Triangles),
            m_outlineVertices(Triangles),
            m_bounds(),
            m_geometryNeedUpdate(true) {
        type = Type::Text;
        if (m_font == nullptr) {
            m_font = c2d_renderer->getFont();
        }
    }


////////////////////////////////////////////////////////////
    void Text::setString(const std::string &string) {
        if (m_string != string) {
            m_string = string;
            m_geometryNeedUpdate = true;
        }
    }


////////////////////////////////////////////////////////////
    void Text::setFont(Font *font) {
        if (m_font != font) {
            m_font = font;
            m_geometryNeedUpdate = true;
        }
    }


////////////////////////////////////////////////////////////
    void Text::setCharacterSize(unsigned int size) {
        if (m_characterSize != size) {
            m_characterSize = size;
            m_geometryNeedUpdate = true;
        }
    }


////////////////////////////////////////////////////////////
    void Text::setStyle(uint32_t style) {
        if (m_style != style) {
            m_style = style;
            m_geometryNeedUpdate = true;
        }
    }

    void Text::setOverflow(uint32_t overflow) {
        if (m_overflow != overflow) {
            m_overflow = overflow;
            m_geometryNeedUpdate = true;
        }
    }

////////////////////////////////////////////////////////////
    void Text::setFillColor(const Color &color) {
        if (color != m_fillColor) {
            m_fillColor = color;

            if (!m_geometryNeedUpdate) {
                for (std::size_t i = 0; i < m_vertices.getVertexCount(); ++i) {
                    m_vertices[i].color = m_fillColor;
                }
                m_vertices.update();
            }
        }
    }

////////////////////////////////////////////////////////////
    void Text::setOutlineColor(const Color &color) {
        if (!m_font->isBmFont() && color != m_outlineColor) {
            m_outlineColor = color;

            if (!m_geometryNeedUpdate) {
                for (std::size_t i = 0; i < m_outlineVertices.getVertexCount(); ++i) {
                    m_outlineVertices[i].color = m_outlineColor;
                }
                m_outlineVertices.update();
            }
        }
    }


////////////////////////////////////////////////////////////
    void Text::setOutlineThickness(float thickness) {
        if (!m_font->isBmFont() && thickness != m_outlineThickness) {
            m_outlineThickness = thickness;
            m_geometryNeedUpdate = true;
        }
    }

/////////////////////////////////////////////////////////////
    void Text::setAlpha(uint8_t alpha, bool recursive) {
        if (alpha != m_fillColor.a) {
            Color color;
            color = m_fillColor;
            color.a = alpha;
            setFillColor(color);
            color = m_outlineColor;
            color.a = alpha;
            setOutlineColor(color);
        }

        if (recursive) {
            C2DObject::setAlpha(alpha, recursive);
        }
    }

    uint8_t Text::getAlpha() {
        return m_fillColor.a;
    }


////////////////////////////////////////////////////////////
    const std::string &Text::getString() const {
        return m_string;
    }


////////////////////////////////////////////////////////////
    Font *Text::getFont() {
        return m_font;
    }


////////////////////////////////////////////////////////////
    unsigned int Text::getCharacterSize() const {
        return m_characterSize;
    }


////////////////////////////////////////////////////////////
    uint32_t Text::getStyle() const {
        return m_style;
    }

    uint32_t Text::getOverflow() const {
        return m_overflow;
    }

////////////////////////////////////////////////////////////
    const Color &Text::getFillColor() const {
        return m_fillColor;
    }


////////////////////////////////////////////////////////////
    const Color &Text::getOutlineColor() const {
        return m_outlineColor;
    }


////////////////////////////////////////////////////////////
    float Text::getOutlineThickness() const {
        return m_outlineThickness;
    }


////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
    FloatRect Text::getLocalBounds() const {
        ensureGeometryUpdate();
        return m_bounds;
    }


////////////////////////////////////////////////////////////
    FloatRect Text::getGlobalBounds() const {
        ensureGeometryUpdate();
        Transform t = transformation * getTransform();
        return t.transformRect(getLocalBounds());
    }


////////////////////////////////////////////////////////////
    void Text::setOrigin(const Origin &origin) {
        m_text_origin = origin;
        float height = m_bounds.height > (float) m_characterSize ? m_bounds.height : (float) m_characterSize;

        switch (origin) {
            case Origin::Left:
                Transformable::setOriginVector(0 - m_outlineThickness,
                                               (height / 2) - m_outlineThickness);
                break;
            case Origin::TopLeft:
                Transformable::setOriginVector(0 - m_outlineThickness,
                                               0 - m_outlineThickness);
                break;
            case Origin::Top:
                Transformable::setOriginVector((m_bounds.width / 2) - m_outlineThickness,
                                               0 - m_outlineThickness);
                break;
            case Origin::TopRight:
                Transformable::setOriginVector(m_bounds.width - m_outlineThickness,
                                               0 - m_outlineThickness);
                break;
            case Origin::Right:
                Transformable::setOriginVector(m_bounds.width - m_outlineThickness,
                                               (height / 2) - m_outlineThickness);
                break;
            case Origin::BottomRight:
                Transformable::setOriginVector(m_bounds.width - m_outlineThickness,
                                               height - m_outlineThickness);
                break;
            case Origin::Bottom:
                Transformable::setOriginVector((m_bounds.width / 2) - m_outlineThickness,
                                               height - m_outlineThickness);
                break;
            case Origin::BottomLeft:
                Transformable::setOriginVector(0 - m_outlineThickness,
                                               height - m_outlineThickness);
                break;
            case Origin::Center:
                Transformable::setOriginVector((m_bounds.width / 2) - m_outlineThickness,
                                               (height / 2) - m_outlineThickness);
                break;
            default:
                break;
        }
    }

    Origin Text::getOrigin() const {
        return m_text_origin;
    }

    void Text::setPosition(float x, float y) {
        Transformable::setPosition(x, y);
    }

    void Text::setPosition(const Vector2f &position) {
        Text::setPosition(position.x, position.y);
    }

////////////////////////////////////////////////////////////

    Vector2f Text::getSize() {
        ensureGeometryUpdate();
        return m_size;
    }

    void Text::setSize(const Vector2f &size) {
        setSize(size.x, size.y);
    }

    void Text::setSize(float width, float height) {
        m_max_size.x = width;
        m_max_size.y = height;
        if (height > 0) {
            setCharacterSize((unsigned int) height);
        }
        m_geometryNeedUpdate = true;
    }

    void Text::setSizeMax(const Vector2f &size) {
        setSizeMax(size.x, size.y);
    }

    void Text::setSizeMax(float width, float height) {
        m_max_size.x = width;
        m_max_size.y = height;
        m_geometryNeedUpdate = true;
    }

    void Text::setLineSpacingModifier(int size) {
        m_line_spacing = size;
    }

    void Text::onUpdate() {
        if (!m_font || m_string.empty()) {
            return;
        }

        Vector2i texSize = m_font->getTexture(m_characterSize)->getTextureSize();
        if (!m_font->isBmFont() && texSize != m_textureSize) {
            m_textureSize = texSize;
            m_geometryNeedUpdate = true;
            ensureGeometryUpdate();
        } else {
            m_textureSize = texSize;
            ensureGeometryUpdate();
        }

        setOrigin(m_text_origin);
    }

    void Text::onDraw(Transform &transform, bool draw) {
        if (!m_font || m_string.empty()) {
            return;
        }

        if (draw) {
            Transform combined = transform * getTransform();
            if (getOutlineThickness() > 0) {
                c2d_renderer->draw(&m_outlineVertices, combined, m_font->getTexture(m_characterSize));
            }
            c2d_renderer->draw(&m_vertices, combined, m_font->getTexture(m_characterSize));
        }
        C2DObject::onDraw(transform, draw);
    }

////////////////////////////////////////////////////////////
Vector2f Text::findCharacterPos(std::size_t index) const {
    // Make sure that we have a valid font
    if (m_font == nullptr)
        return {};

    // Convert UTF-8 string to UTF-32 for proper character indexing
	    // Convert UTF-8 to UTF-32
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    std::u32string utf32String = converter.from_bytes(m_string);

    // Adjust the index if it's out of range
    if (index > utf32String.length())
        index = utf32String.length();

    // Precompute the variables needed by the algorithm
    bool bold = (m_style & Bold) != 0;
    auto hspace = static_cast<float>(m_font->getGlyph(U' ', m_characterSize, bold).advance);
    auto vspace = static_cast<float>(m_font->getLineSpacing(m_characterSize)) + (float)m_line_spacing;

    // Compute the position
    Vector2f position;
    uint32_t prevChar = 0;
    for (std::size_t i = 0; i < index; ++i) {
        uint32_t curChar = utf32String[i];

        // Apply the kerning offset
        position.x += static_cast<float>(m_font->getKerning(prevChar, curChar, m_characterSize, bold));
        prevChar = curChar;

        // Handle special characters
        switch (curChar) {
            case U' ':
                position.x += hspace;
                continue;
            case U'\t':
                position.x += hspace * 4;
                continue;
            case U'\n':
                position.y += vspace;
                position.x = 0;
                continue;
            default:
                break;
        }

        // For regular characters, add the advance offset of the glyph
        position.x += static_cast<float>(m_font->getGlyph(curChar, m_characterSize, bold).advance);
    }

    // Transform the position to global coordinates
    position = getTransform().transformPoint(position);

    return position;
}

void Text::ensureGeometryUpdate() const {
    if (!m_geometryNeedUpdate) return;
    m_geometryNeedUpdate = false;

    if (!m_font || m_string.empty()) return;

    // 清除之前的几何数据
    m_vertices.clear();
    m_outlineVertices.clear();
    m_bounds = FloatRect();

    // 获取样式信息
    bool bold = m_style & Bold;
    bool underlined = m_style & Underlined;
    bool strikeThrough = m_style & StrikeThrough;
    float italic = (m_style & Italic) ? 0.208f : 0.f;
    float underlineOffset = m_font->getUnderlinePosition(m_characterSize);
    float underlineThickness = m_font->getUnderlineThickness(m_characterSize);

    // 计算行高和空格宽度
    float hspace = static_cast<float>(m_font->getGlyph(U' ', m_characterSize, bold).advance);
    float vspace = static_cast<float>(m_font->getLineSpacing(m_characterSize)) + m_line_spacing;

    // 初始化位置
    float x = 0.f;
    float y = static_cast<float>(m_characterSize);
    
    // 应用字体偏移
    float scale = static_cast<float>(m_characterSize) / C2D_DEFAULT_CHAR_SIZE;
    x += m_font->getOffset().x * scale;
    y += m_font->getOffset().y * scale;

    // 边界计算
    float minX = x, minY = y;
    float maxX = x, maxY = y;
    uint32_t prevChar = 0;

    // 转换为UTF-32处理
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
    std::u32string utf32String = converter.from_bytes(m_string);
    
    // 判断是否为单行模式，转义下原定义懒得修改后面的代码了
	bool singleLineMode = m_overflow ? false : true;
	
    // 计算可用宽度
    float availableWidth = std::numeric_limits<float>::max();
    if (m_max_size.x > 0) availableWidth = m_max_size.x;
    if (m_bounds.width > 0 && m_bounds.width < availableWidth) availableWidth = m_bounds.width;

    // 预计算省略号宽度
    const Glyph& ellipsisGlyph = m_font->getGlyph(U'.', m_characterSize, bold);
    float ellipsisWidth = ellipsisGlyph.advance * 3; // 三个点

    bool truncated = false;

    // 逐字符处理
    for (size_t i = 0; i < utf32String.size(); ++i) {
        uint32_t curChar = utf32String[i];
        
        // 应用字距调整
        x += m_font->getKerning(prevChar, curChar, m_characterSize, bold);
        prevChar = curChar;

        // 处理特殊字符
        switch (curChar) {
            case U' ': {
                float newX = x + hspace;
                if (!singleLineMode || newX <= availableWidth) {
                    x = newX;
                }
                continue;
            }
            case U'\t': {
                float newX = x + hspace * 4;
                if (!singleLineMode || newX <= availableWidth) {
                    x = newX;
                }
                continue;
            }
            case U'\n':
                if (singleLineMode) {
                    // 单行模式下将换行符视为空格
                    float newX = x + hspace;
                    if (newX <= availableWidth) {
                        x = newX;
                    }
                } else {
                    // 多行模式下正常换行
                    y += vspace;
                    x = m_font->getOffset().x * scale;
                    
                    // 检查是否超出垂直边界
                    float availableHeight = std::numeric_limits<float>::max();
                    if (m_max_size.y > 0) availableHeight = m_max_size.y;
                    if (m_bounds.height > 0 && m_bounds.height < availableHeight) availableHeight = m_bounds.height;
                    
                    if (y > availableHeight) {
                        truncated = true;
                        goto add_ellipsis;
                    }
                }
                continue;
            default:
                break;
        }

        // 获取当前字符的字形信息
        const Glyph& glyph = m_font->getGlyph(curChar, m_characterSize, bold);
        
        // 检查是否超出水平边界（考虑可能的省略号）
        float newX = x + glyph.advance;
        bool willTruncate = (singleLineMode && (newX + ellipsisWidth) > availableWidth) ||
                           (!singleLineMode && newX > availableWidth);

        if (willTruncate) {
            if (singleLineMode) {
                // 单行模式需要检查是否可以添加省略号
                if ((x + ellipsisWidth) <= availableWidth) {
                    // 有足够空间添加省略号
                    truncated = true;
                    goto add_ellipsis;
                } else {
                    // 没有足够空间，直接截断
                    break;
                }
            } else {
                // 多行模式换行
                y += vspace;
                x = m_font->getOffset().x * scale;
                newX = x + glyph.advance;
                
                // 检查是否超出垂直边界
                float availableHeight = std::numeric_limits<float>::max();
                if (m_max_size.y > 0) availableHeight = m_max_size.y;
                if (m_bounds.height > 0 && m_bounds.height < availableHeight) availableHeight = m_bounds.height;
                
                if (y > availableHeight) {
                    truncated = true;
                    goto add_ellipsis;
                }
            }
        }

        // 添加轮廓(如果有)
        if (m_outlineThickness != 0) {
            const Glyph& outlineGlyph = m_font->getGlyph(curChar, m_characterSize, bold, m_outlineThickness);
            addGlyphQuad(m_outlineVertices, m_textureSize, {x, y}, m_outlineColor, outlineGlyph, italic);
            
            // 更新带轮廓的边界
            float left = outlineGlyph.bounds.left - m_outlineThickness;
            float top = outlineGlyph.bounds.top - m_outlineThickness;
            float right = outlineGlyph.bounds.left + outlineGlyph.bounds.width + m_outlineThickness;
            float bottom = outlineGlyph.bounds.top + outlineGlyph.bounds.height + m_outlineThickness;
            
            minX = std::min(minX, x + left - italic * bottom);
            maxX = std::max(maxX, x + right - italic * top);
            minY = std::min(minY, y + top);
            maxY = std::max(maxY, y + bottom);
        }

        // 添加主字形
        addGlyphQuad(m_vertices, m_textureSize, {x, y}, m_fillColor, glyph, italic);

        // 更新无轮廓的边界
        float left = glyph.bounds.left;
        float top = glyph.bounds.top;
        float right = glyph.bounds.left + glyph.bounds.width;
        float bottom = glyph.bounds.top + glyph.bounds.height;
        
        minX = std::min(minX, x + left - italic * bottom);
        maxX = std::max(maxX, x + right - italic * top);
        minY = std::min(minY, y + top);
        maxY = std::max(maxY, y + bottom);
        
        // 移动到下一个字符位置
        x = newX;
    }

add_ellipsis:
    // 如果被截断且是单行模式，添加省略号
    if (truncated && singleLineMode && (x + ellipsisWidth) <= availableWidth) {
        // 添加三个点作为省略号
        for (int i = 0; i < 3; ++i) {
            const Glyph& dotGlyph = m_font->getGlyph(U'.', m_characterSize, bold);
            
            // 添加轮廓
            if (m_outlineThickness != 0) {
                const Glyph& outlineDotGlyph = m_font->getGlyph(U'.', m_characterSize, bold, m_outlineThickness);
                addGlyphQuad(m_outlineVertices, m_textureSize, {x, y}, m_outlineColor, outlineDotGlyph, italic);
            }
            
            // 添加主字形
            addGlyphQuad(m_vertices, m_textureSize, {x, y}, m_fillColor, dotGlyph, italic);
            
            // 更新边界
            float left = dotGlyph.bounds.left;
            float top = dotGlyph.bounds.top;
            float right = dotGlyph.bounds.left + dotGlyph.bounds.width;
            float bottom = dotGlyph.bounds.top + dotGlyph.bounds.height;
            
            minX = std::min(minX, x + left - italic * bottom);
            maxX = std::max(maxX, x + right - italic * top);
            minY = std::min(minY, y + top);
            maxY = std::max(maxY, y + bottom);
            
            x += dotGlyph.advance;
        }
    }

    // 添加行尾的下划线
    if (underlined && x > 0) {
        addLine(m_vertices, m_textureSize, x, y, m_fillColor, underlineOffset, underlineThickness);
        if (m_outlineThickness != 0) {
            addLine(m_outlineVertices, m_textureSize, x, y, 
                   m_outlineColor, underlineOffset, underlineThickness, m_outlineThickness);
        }
    }
    
    // 添加行尾的删除线
    if (strikeThrough && x > 0) {
        FloatRect xBounds = m_font->getGlyph(U'x', m_characterSize, bold).bounds;
        float strikeThroughOffset = xBounds.top + xBounds.height / 2.f;
        
        addLine(m_vertices, m_textureSize, x, y, m_fillColor, strikeThroughOffset, underlineThickness);
        if (m_outlineThickness != 0) {
            addLine(m_outlineVertices, m_textureSize, x, y,
                   m_outlineColor, strikeThroughOffset, underlineThickness, m_outlineThickness);
        }
    }

    // 更新最终边界
    m_bounds = {minX, minY, maxX - minX, maxY - minY};
    m_size = {m_bounds.width, m_bounds.height};

    m_vertices.update();
    m_outlineVertices.update();
}


} // namespace sf
