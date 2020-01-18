#include <renderlib/render_context.h>
#include <renderlib/render_engine.h>
#include <renderlib/render_api.h>

#include <iolib/window.h>

#include <type_traits>
#include <cassert>

mooned::render::RenderContext::RenderContext(mem::allocator& alloc, mooned::io::IOSystem* iosystem)
    : _allocator{ alloc }
    , _details{ }
    , _render_engine{ nullptr }
{
    // Create a render_engine
    _render_engine = mooned::render::RenderSystemTypes{ _allocator }.create_engine();

    // Fill the details structure
    _details.engine.backend = _render_engine->backend();
    _details.engine.version = _render_engine->version();

    // Initialize the drawing devices on the IO system using the given render engine backed
    auto* window = mooned::io::render_window(iosystem);
    assert(nullptr != window);

    // Initialize the engine with the given window
    _render_engine->initialize(window);
}

mooned::render::RenderContext::~RenderContext()
{
    mooned::render::RenderSystemTypes{ _allocator }.destroy_engine(_render_engine);
}

const mooned::render::RenderContextDetails& mooned::render::RenderContext::details() const
{
    return _details;
}

mooned::render::RenderEngine* mooned::render::RenderContext::render_engine() const
{
    assert(nullptr != _render_engine);
    return _render_engine;
}

bool mooned::render::RenderContext::valid() const
{
    return nullptr != _render_engine;
}

void mooned::render::RenderContext::swap()
{
    _render_engine->swap();
}
