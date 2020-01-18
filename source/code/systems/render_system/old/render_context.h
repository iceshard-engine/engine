#pragma once
#include <memsys/allocator.h>
#include <renderlib/render_engine.h>

#include <iolib/system.h>

namespace mooned::render
{

struct RenderEngineInfo
{
    mooned::io::RenderBackend backend;
    RenderEngineVersion version;
};

struct RenderContextDetails
{
    RenderEngineInfo engine;
};

// The main render object, which handles the lifetime of a render engine.
class RenderContext final
{
public:
    RenderContext(mem::allocator& alloc, mooned::io::IOSystem* iosystem);
    ~RenderContext();

    //! Returns detailed information about this render context.
    const RenderContextDetails& details() const;

    //! Returns a pointer to the created render engine
    RenderEngine* render_engine() const;

    //! Returns 'true' if the context was created successfully and a render engine is available.
    bool valid() const;

    //! Swaps the back buffer of the default render window.
    void swap();

private:
    mem::allocator& _allocator;

    RenderContextDetails _details;
    RenderEngine* _render_engine;
};

}