#include <renderlib/render_api.h>

#include <unordered_map>
#include <cassert>

//namespace mooned::render::detail
//{
//
//using RenderSystemFactoryMap = std::unordered_map<RenderSystemClass, RenderSystemObjectFactory>;
//using RenderSystemDestructorMap = std::unordered_map<RenderSystemClass, RenderSystemObjectDestructor>;
//
//static RenderSystemFactoryMap& get_render_system_factory_map()
//{
//    static RenderSystemFactoryMap factory_map;
//    return factory_map;
//}
//
//static RenderSystemDestructorMap& get_render_system_destructor_map()
//{
//    static RenderSystemDestructorMap factory_map;
//    return factory_map;
//}
//
//}
//
//mooned::render::RegisterRenderSystemClass::RegisterRenderSystemClass(RenderSystemClass render_class, RenderSystemObjectFactory factory, RenderSystemObjectDestructor destructor)
//{
//    static auto& factory_map = mooned::render::detail::get_render_system_factory_map();
//    static auto& destructor_map = mooned::render::detail::get_render_system_destructor_map();
//
//    // We should only register a render system class once!
//    assert(factory_map.count(render_class) == 0);
//    assert(factory_map.count(render_class) == 0);
//
//    // A factory needs to be a valid pointer
//    assert(nullptr != factory);
//    assert(nullptr != destructor);
//
//    // Register the class factory
//    factory_map.emplace(render_class, factory);
//    destructor_map.emplace(render_class, destructor);
//}

mooned::render::RenderSystemTypes::RenderSystemTypes(mem::allocator& alloc)
    : _allocator{ alloc }
{
}

mooned::render::RenderSystemTypes::~RenderSystemTypes()
{
}
//
//mooned::render::RenderEngine* mooned::render::RenderSystemTypes::create_engine()
//{
//    static auto& map = mooned::render::detail::get_render_system_factory_map();
//    assert(map.count(RenderSystemClass::ENGINE_CLASS) == 1);
//
//    // Get the factory
//    RenderSystemObjectFactory factory = map.at(RenderSystemClass::ENGINE_CLASS);
//    assert(nullptr != factory);
//
//    // Create the object and reinterpret_cast to the given interface
//    return reinterpret_cast<mooned::render::RenderEngine*>(factory(_allocator));
//}
//
//void mooned::render::RenderSystemTypes::destroy_engine(RenderEngine* engine)
//{
//    static auto& map = mooned::render::detail::get_render_system_destructor_map();
//    assert(map.count(RenderSystemClass::ENGINE_CLASS) == 1);
//
//    // Get the destructor
//    RenderSystemObjectDestructor destructor = map.at(RenderSystemClass::ENGINE_CLASS);
//    assert(nullptr != destructor);
//
//    // Destroy the object
//    destructor(_allocator, engine);
//}
//
//mooned::render::RenderShader* mooned::render::RenderSystemTypes::create_shader()
//{
//    static auto& map = mooned::render::detail::get_render_system_factory_map();
//    assert(map.count(RenderSystemClass::SHADER_CLASS) == 1);
//
//    // Get the factory
//    RenderSystemObjectFactory factory = map.at(RenderSystemClass::SHADER_CLASS);
//    assert(nullptr != factory);
//
//    // Create the object and reinterpret_cast to the given interface
//    return reinterpret_cast<mooned::render::RenderShader*>(factory(_allocator));
//}
//
//void mooned::render::RenderSystemTypes::destroy_shader(RenderShader* shader)
//{
//    static auto& map = mooned::render::detail::get_render_system_destructor_map();
//    assert(map.count(RenderSystemClass::SHADER_CLASS) == 1);
//
//    // Get the destructor
//    RenderSystemObjectDestructor destructor = map.at(RenderSystemClass::SHADER_CLASS);
//    assert(nullptr != destructor);
//
//    // Destroy the object
//    destructor(_allocator, shader);
//}
//
//mooned::render::RenderTexture* mooned::render::RenderSystemTypes::create_texture()
//{
//    static auto& map = mooned::render::detail::get_render_system_factory_map();
//    assert(map.count(RenderSystemClass::TEXTURE_CLASS) == 1);
//
//    // Get the factory
//    RenderSystemObjectFactory factory = map.at(RenderSystemClass::TEXTURE_CLASS);
//    assert(nullptr != factory);
//
//    // Create the object and reinterpret_cast to the given interface
//    return reinterpret_cast<mooned::render::RenderTexture*>(factory(_allocator));
//}
//
//void mooned::render::RenderSystemTypes::destroy_texture(RenderTexture* texture)
//{
//    static auto& map = mooned::render::detail::get_render_system_destructor_map();
//    assert(map.count(RenderSystemClass::TEXTURE_CLASS) == 1);
//
//    // Get the destructor
//    RenderSystemObjectDestructor destructor = map.at(RenderSystemClass::TEXTURE_CLASS);
//    assert(nullptr != destructor);
//
//    // Destroy the object
//    destructor(_allocator, texture);
//}
//
//mooned::render::RenderVertexArray* mooned::render::RenderSystemTypes::create_vertex_array()
//{
//    static auto& map = mooned::render::detail::get_render_system_factory_map();
//    assert(map.count(RenderSystemClass::VERTEX_ARRAY_CLASS) == 1);
//
//    // Get the factory
//    RenderSystemObjectFactory factory = map.at(RenderSystemClass::VERTEX_ARRAY_CLASS);
//    assert(nullptr != factory);
//
//    // Create the object and reinterpret_cast to the given interface
//    return reinterpret_cast<mooned::render::RenderVertexArray*>(factory(_allocator));
//}
//
//void mooned::render::RenderSystemTypes::destroy_vertex_array(RenderVertexArray* vertex_array)
//{
//    static auto& map = mooned::render::detail::get_render_system_destructor_map();
//    assert(map.count(RenderSystemClass::VERTEX_ARRAY_CLASS) == 1);
//
//    // Get the destructor
//    RenderSystemObjectDestructor destructor = map.at(RenderSystemClass::VERTEX_ARRAY_CLASS);
//    assert(nullptr != destructor);
//
//    // Destroy the object
//    destructor(_allocator, vertex_array);
//}
//
//mooned::render::RenderCommandBuffer* mooned::render::RenderSystemTypes::create_command_buffer()
//{
//    static auto& map = mooned::render::detail::get_render_system_factory_map();
//    assert(map.count(RenderSystemClass::COMMAND_BUFFER_CLASS) == 1);
//
//    // Get the factory
//    RenderSystemObjectFactory factory = map.at(RenderSystemClass::COMMAND_BUFFER_CLASS);
//    assert(nullptr != factory);
//
//    // Create the object and reinterpret_cast to the given interface
//    return reinterpret_cast<mooned::render::RenderCommandBuffer*>(factory(_allocator));
//}
//
//void mooned::render::RenderSystemTypes::destroy_command_buffer(RenderCommandBuffer* command_buffer)
//{
//    static auto& map = mooned::render::detail::get_render_system_destructor_map();
//    assert(map.count(RenderSystemClass::COMMAND_BUFFER_CLASS) == 1);
//
//    // Get the destructor
//    RenderSystemObjectDestructor destructor = map.at(RenderSystemClass::COMMAND_BUFFER_CLASS);
//    assert(nullptr != destructor);
//
//    // Destroy the object
//    destructor(_allocator, command_buffer);
//}
