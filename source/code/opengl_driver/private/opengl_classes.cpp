#pragma once
#include "opengl_engine.h"

#include <renderlib/render_api.h>

// A renderer 'engine' class
//void* render_opengl_create_engine(mem::allocator& alloc)
//{
//    return MAKE_NEW(alloc, mooned::render::opengl::RenderOpenGlEngine);
//}
//
//void render_opengl_destroy_engine(mem::allocator& alloc, void* ptr)
//{
//    auto* engine = reinterpret_cast<mooned::render::opengl::RenderOpenGlEngine*>(ptr);
//    MAKE_DELETE(alloc, RenderOpenGlEngine, engine);
//}
//

mooned::render::RenderEngine* mooned::render::RenderSystemTypes::create_engine()
{
    return MAKE_NEW(_allocator, mooned::render::opengl::OpenGlRenderEngine);
}

void mooned::render::RenderSystemTypes::destroy_engine(RenderEngine* ptr)
{
    auto* engine = reinterpret_cast<mooned::render::opengl::OpenGlRenderEngine*>(ptr);
    MAKE_DELETE(_allocator, OpenGlRenderEngine, engine);
}

//
//// Register all classes
//void mooned::render::register_render_classes()
//{
//    mooned::render::RegisterRenderSystemClass register_render_engine_class(
//        mooned::render::RenderSystemClass::ENGINE_CLASS,
//        render_opengl_create_engine,
//        render_opengl_destroy_engine
//    );
//}