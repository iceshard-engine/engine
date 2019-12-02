#pragma once

namespace mooned::render
{

//! Defines all available element types
enum class ElementType
{
    UNSIGNED_SHORT
};

//! Describes a set of render options which can be enabled / disabled during the render stage.
enum class RenderOption
{
    BLEND,
    DEPTH_TEST,
    CULL_FACE,
    SCISSOR_TEST,
    MULTI_SAMPLE,
};

//! Describes available clear flags
enum class ClearFlags
{
    NONE = 0x00,
    COLOR = 0x01,
    DEPTH = 0x02,
    DEPTH_COLOR = 0x03
};

//! Describes available blend equation values
enum class BlendEquation
{
    ADD,
};

//! Describes available blend sources
enum class BlendFactor
{
    ZERO,
    ONE,
    SRC_COLOR,
    ONE_MINUS_SRC_COLOR,
    DST_COLOR,
    ONE_MINUS_DST_COLOR,
    SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA,
    DST_ALPHA,
    ONE_MINUS_DST_ALPHA,
};

//! Defines all available texture slots
enum class TextureSlot
{
    SLOT0,
    SLOT1,
    SLOT2,
    SLOT3,
    SLOT4,
    SLOT5,
    SLOT6
};

//! Defines all available draw procedures
enum class DrawFunction
{
    WIREFRAME,
    TRIANGLES,
    TRIANGLE_FAN,
};

}
