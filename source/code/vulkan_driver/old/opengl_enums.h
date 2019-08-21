#pragma once
#include <renderlib/render_enums.h>
#include <renderlib/render_buffer.h>
#include <renderlib/render_program.h>
#include <renderlib/render_texture.h>

#include <GL/glew.h>
#include <GL/GL.h>

namespace mooned::render
{

//! Returns the name of the given element type.
const char* tostring(ElementType element);

//! Returns the name of the given option.
const char* tostring(RenderOption option);

//! Returns the name of the given flag combination.
const char* tostring(ClearFlags flags);

//! Returns the name of the given blend equation.
const char* tostring(BlendEquation equation);

//! Returns the name of the given blend source.
const char* tostring(BlendFactor factor);

//! Returns the name of the given texture type.
const char* tostring(TextureDetails::Type type);

//! Returns the name of the given texture slot.
const char* tostring(TextureSlot option);

//! Returns the name of the given buffer target.
const char* tostring(BufferTarget buffer);

//! Returns the name of the given program shader stage.
const char* tostring(ShaderProgram::Stage stage);

//! Returns the name of the given draw function.
const char* tostring(DrawFunction function);

// Returns the OpenGL enum value of the given element.
GLenum togl(ElementType element);

//! Returns the OpengGL enum value of the given option.
GLenum togl(RenderOption option);

//! Returns the OpenGL flag combination of the given value.
GLenum togl(ClearFlags flags);

//! Returns the OpenGL enum value of the given blend equation.
GLenum togl(BlendEquation equation);

//! Returns the OpenGL enum value of the given blend source.
GLenum togl(BlendFactor factor);

//! Returns the OpenGL enum value of the given texture type.
GLenum togl(TextureDetails::Type type);

//! Returns the OpenGL enum value of the given texture slot.
GLenum togl(TextureSlot slot);

//! Returns the OpenGL enum value of the given buffer target.
GLenum togl(BufferTarget buffer);

//! Returns the OpenGL enum value of the program shader stage.
GLenum togl(ShaderProgram::Stage stage);

//! Returns the OpenGL enum value of the given draw function.
GLenum togl(DrawFunction function);

}
