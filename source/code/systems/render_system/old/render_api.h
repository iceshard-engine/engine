#pragma once
#include <core/allocator.hxx>

namespace mooned::render
{
//
//// All render system classes which should be registered prior to creating a render context
//enum class RenderSystemClass
//{
//    ENGINE_CLASS,
//    SHADER_CLASS,
//    TEXTURE_CLASS,
//    VERTEX_ARRAY_CLASS,
//    COMMAND_BUFFER_CLASS,
//};
//
//using RenderSystemObjectFactory = void*(*)(mem::allocator& alloc);
//using RenderSystemObjectDestructor = void(*)(mem::allocator& alloc, void*);
//
//void register_render_classes();
//
//// Registers a factory for the given render system class type
//class RegisterRenderSystemClass
//{
//public:
//    RegisterRenderSystemClass(RenderSystemClass render_class, RenderSystemObjectFactory factory, RenderSystemObjectDestructor dest);
//};

// Class declarations
class RenderEngine;
class RenderShader;
class RenderTexture;
class RenderVertexArray;
class RenderCommandBuffer;

// Creates a factory object which knows how to create specific render system objects.
class RenderSystemTypes
{
public:
    RenderSystemTypes(mem::allocator& alloc);
    ~RenderSystemTypes();

    RenderEngine* create_engine();
    void destroy_engine(RenderEngine* engine);

    RenderShader* create_shader();
    void destroy_shader(RenderShader* engine);

    RenderTexture* create_texture();
    void destroy_texture(RenderTexture* engine);

    RenderVertexArray* create_vertex_array();
    void destroy_vertex_array(RenderVertexArray* engine);

    RenderCommandBuffer* create_command_buffer();
    void destroy_command_buffer(RenderCommandBuffer* engine);

private:
    mem::allocator& _allocator;
};

}
