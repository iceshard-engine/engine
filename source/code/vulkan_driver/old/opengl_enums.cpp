#include "opengl_enums.h"

#include <cassert>

const char* mooned::render::tostring(ElementType element)
{
    switch (element)
    {
#define CASE(name) case name: return #name
        CASE(ElementType::UNSIGNED_SHORT);
#undef CASE
    }

    assert(false);
    return "ElementType::<UNKNOWN>";
}

const char* mooned::render::tostring(RenderOption option)
{
    switch (option)
    {
#define CASE(name) case name: return #name
        CASE(RenderOption::BLEND);
        CASE(RenderOption::DEPTH_TEST);
        CASE(RenderOption::CULL_FACE);
        CASE(RenderOption::SCISSOR_TEST);
        CASE(RenderOption::MULTI_SAMPLE);
#undef CASE
    }

    assert(false);
    return "RenderOption::<UNKNOWN>";
}

const char* mooned::render::tostring(ClearFlags flags)
{
    switch (flags)
    {
#define CASE(name) case name: return #name
        CASE(ClearFlags::NONE);
        CASE(ClearFlags::DEPTH);
        CASE(ClearFlags::COLOR);
        CASE(ClearFlags::DEPTH_COLOR);
#undef CASE
    }

    assert(false);
    return "ClearFlags::<UNKNOWN>";
}

const char* mooned::render::tostring(BlendEquation equation)
{
    switch (equation)
    {
#define CASE(name) case name: return #name
        CASE(BlendEquation::ADD);
#undef CASE
    }

    assert(false);
    return "BlendEquation::<UNKNOWN>";
}

const char* mooned::render::tostring(BlendFactor factor)
{
    switch (factor)
    {
#define CASE(name) case name: return #name
        CASE(BlendFactor::ZERO);
        CASE(BlendFactor::ONE);
        CASE(BlendFactor::SRC_COLOR);
        CASE(BlendFactor::ONE_MINUS_SRC_COLOR);
        CASE(BlendFactor::DST_COLOR);
        CASE(BlendFactor::ONE_MINUS_DST_COLOR);
        CASE(BlendFactor::SRC_ALPHA);
        CASE(BlendFactor::ONE_MINUS_SRC_ALPHA);
        CASE(BlendFactor::DST_ALPHA);
        CASE(BlendFactor::ONE_MINUS_DST_ALPHA);
#undef CASE
    }

    assert(false);
    return "BlendFactor::<UNKNOWN>";
}

const char* mooned::render::tostring(TextureDetails::Type type)
{
    switch (type)
    {
#define CASE(name) case name: return #name
        CASE(TextureDetails::Type::TEXTURE_1D);
        CASE(TextureDetails::Type::TEXTURE_2D);
        CASE(TextureDetails::Type::TEXTURE_2D_MULTISAMPLE);
        CASE(TextureDetails::Type::TEXTURE_3D);
#undef CASE
    }

    assert(false);
    return "TextureDetails::Type::<UNKNOWN>";
}

const char* mooned::render::tostring(TextureSlot slot)
{
    switch (slot)
    {
#define CASE(e, name) case e: return #name
        CASE(TextureSlot::SLOT0, "TextureSlot<0>");
        CASE(TextureSlot::SLOT1, "TextureSlot<1>");
        CASE(TextureSlot::SLOT2, "TextureSlot<2>");
        CASE(TextureSlot::SLOT3, "TextureSlot<3>");
        CASE(TextureSlot::SLOT4, "TextureSlot<4>");
        CASE(TextureSlot::SLOT5, "TextureSlot<5>");
        CASE(TextureSlot::SLOT6, "TextureSlot<6>");
#undef CASE
    }

    assert(false);
    return "TextureSlot<UNKNOWN>";
}

const char* mooned::render::tostring(BufferTarget buffer)
{
    switch (buffer)
    {
#define CASE(name) case name: return #name
        CASE(BufferTarget::ARRAY_BUFFER);
        CASE(BufferTarget::ELEMENT_ARRAY_BUFFER);
        CASE(BufferTarget::UNIFORM_BUFFER);
#undef CASE
    }

    assert(false);
    return "BufferTarget::<UNKNOWN>";
}

const char* mooned::render::tostring(ShaderProgram::Stage stage)
{
    switch (stage)
    {
#define CASE(name) case name: return #name
        CASE(ShaderProgram::Stage::VERTEX);
        CASE(ShaderProgram::Stage::FRAGMENT);
        CASE(ShaderProgram::Stage::COMPUTE);
        CASE(ShaderProgram::Stage::GEOMETRY);
#undef CASE
    }

    assert(false);
    return "ShaderProgram::Stage::<UNKNOWN>";
}

const char* mooned::render::tostring(DrawFunction function)
{
    switch (function)
    {
#define CASE(name) case name: return #name
        CASE(DrawFunction::WIREFRAME);
        CASE(DrawFunction::TRIANGLES);
        CASE(DrawFunction::TRIANGLE_FAN);
#undef CASE
    }

    assert(false);
    return "DrawFunction::<UNKNOWN>";
}

GLenum mooned::render::togl(ElementType element)
{
    GLenum result = GL_INVALID_ENUM;

    switch (element)
    {
#define CASE(e, gl) case e: result = gl; break
        CASE(ElementType::UNSIGNED_SHORT, GL_UNSIGNED_SHORT);
#undef CASE
    }

    assert(result != GL_INVALID_ENUM);
    return result;
}

GLenum mooned::render::togl(RenderOption option)
{
    GLenum result = GL_INVALID_ENUM;
    switch (option)
    {
#define CASE(e, gl) case e: result = gl; break
        CASE(RenderOption::BLEND, GL_BLEND);
        CASE(RenderOption::DEPTH_TEST, GL_DEPTH_TEST);
        CASE(RenderOption::CULL_FACE, GL_CULL_FACE);
        CASE(RenderOption::SCISSOR_TEST, GL_SCISSOR_TEST);
        CASE(RenderOption::MULTI_SAMPLE, GL_MULTISAMPLE);
#undef CASE
    }

    assert(result != GL_INVALID_ENUM);
    return result;
}

GLenum mooned::render::togl(ClearFlags flags)
{
    GLenum result = GL_INVALID_ENUM;
    switch (flags)
    {
#define CASE(e, gl) case e: result = gl; break
        CASE(ClearFlags::NONE, GL_NONE);
        CASE(ClearFlags::DEPTH, GL_DEPTH_BUFFER_BIT);
        CASE(ClearFlags::COLOR, GL_COLOR_BUFFER_BIT);
        CASE(ClearFlags::DEPTH_COLOR, GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
#undef CASE
    }

    assert(result != GL_INVALID_ENUM);
    return result;
}

GLenum mooned::render::togl(BlendEquation equation)
{
    GLenum result = GL_INVALID_ENUM;
    switch (equation)
    {
#define CASE(e, gl) case e: result = gl; break
        CASE(BlendEquation::ADD, GL_FUNC_ADD);
#undef CASE
    }

    assert(result != GL_INVALID_ENUM);
    return result;
}

GLenum mooned::render::togl(BlendFactor factor)
{
    GLenum result = GL_INVALID_ENUM;
    switch (factor)
    {
#define CASE(e, gl) case e: result = gl; break
        CASE(BlendFactor::ZERO, GL_ZERO);
        CASE(BlendFactor::ONE, GL_ONE);
        CASE(BlendFactor::SRC_COLOR, GL_SRC_COLOR);
        CASE(BlendFactor::ONE_MINUS_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
        CASE(BlendFactor::DST_COLOR, GL_DST_COLOR);
        CASE(BlendFactor::ONE_MINUS_DST_COLOR, GL_ONE_MINUS_DST_COLOR);
        CASE(BlendFactor::SRC_ALPHA, GL_SRC_ALPHA);
        CASE(BlendFactor::ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        CASE(BlendFactor::DST_ALPHA, GL_DST_ALPHA);
        CASE(BlendFactor::ONE_MINUS_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);
#undef CASE
    }

    assert(result != GL_INVALID_ENUM);
    return result;
}

GLenum mooned::render::togl(TextureDetails::Type type)
{
    GLenum result = GL_INVALID_ENUM;

    switch (type)
    {
#define CASE(e, gl) case e: result = gl; break
        CASE(TextureDetails::Type::TEXTURE_1D, GL_TEXTURE_1D);
        CASE(TextureDetails::Type::TEXTURE_2D, GL_TEXTURE_2D);
        CASE(TextureDetails::Type::TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_2D_MULTISAMPLE);
        CASE(TextureDetails::Type::TEXTURE_3D, GL_TEXTURE_3D);
#undef CASE
    default: break;
    }

    assert(result != GL_INVALID_ENUM);
    return result;
}

GLenum mooned::render::togl(TextureSlot slot)
{
    GLenum result = GL_INVALID_ENUM;

    switch (slot)
    {
#define CASE(e, gl) case e: result = gl; break
        CASE(TextureSlot::SLOT0, GL_TEXTURE0);
        CASE(TextureSlot::SLOT1, GL_TEXTURE1);
        CASE(TextureSlot::SLOT2, GL_TEXTURE2);
        CASE(TextureSlot::SLOT3, GL_TEXTURE3);
        CASE(TextureSlot::SLOT4, GL_TEXTURE4);
        CASE(TextureSlot::SLOT5, GL_TEXTURE5);
        CASE(TextureSlot::SLOT6, GL_TEXTURE6);
#undef CASE
    }

    assert(result != GL_INVALID_ENUM);
    return result;
}

GLenum mooned::render::togl(BufferTarget buffer)
{
    GLenum result = GL_INVALID_ENUM;

    switch (buffer)
    {
#define CASE(e, gl) case e: result = gl; break
        CASE(BufferTarget::ARRAY_BUFFER, GL_ARRAY_BUFFER);
        CASE(BufferTarget::ELEMENT_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER);
        CASE(BufferTarget::UNIFORM_BUFFER, GL_UNIFORM_BUFFER);
#undef CASE
    }

    assert(result != GL_INVALID_ENUM);
    return result;
}

GLenum mooned::render::togl(ShaderProgram::Stage stage)
{
    GLenum result = GL_INVALID_ENUM;

    switch (stage)
    {
#define CASE(e, gl) case e: result = gl; break
        CASE(ShaderProgram::Stage::VERTEX, GL_VERTEX_SHADER_BIT);
        CASE(ShaderProgram::Stage::FRAGMENT, GL_FRAGMENT_SHADER_BIT);
        CASE(ShaderProgram::Stage::COMPUTE, GL_COMPUTE_SHADER_BIT);
        CASE(ShaderProgram::Stage::GEOMETRY, GL_GEOMETRY_SHADER_BIT);
#undef CASE
    }

    assert(result != GL_INVALID_ENUM);
    return result;
}

GLenum mooned::render::togl(DrawFunction function)
{
    GLenum result = GL_INVALID_ENUM;

    switch (function)
    {
#define CASE(e, gl) case e: result = gl; break
        CASE(DrawFunction::WIREFRAME, GL_LINE_LOOP);
        CASE(DrawFunction::TRIANGLES, GL_TRIANGLES);
        CASE(DrawFunction::TRIANGLE_FAN, GL_TRIANGLE_FAN);
#undef CASE
    }

    assert(result != GL_INVALID_ENUM);
    return result;
}
