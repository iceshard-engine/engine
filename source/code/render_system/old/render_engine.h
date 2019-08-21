#pragma once
#include <kernel/types.h>
#include <iolib/window.h>

namespace mooned::render
{

using RenderEngineVersion = int32_t;

class CommandBuffer;

void initialize_window(mooned::io::Window* window, mooned::io::RenderBackend backend);

//! Describes the main rendering interface which controls all rendering objects of the current render context.
class RenderEngine
{
public:
    virtual void initialize(mooned::io::Window* window) = 0;

    //! Executes all commands from the given command buffer
    virtual void execute(const CommandBuffer& command_buffer) = 0;

    virtual void swap() = 0;

    virtual mooned::io::RenderBackend backend() const = 0;
    virtual RenderEngineVersion version() const = 0;

protected:
    // Don't allow a user to delete this object.
    virtual ~RenderEngine() = default;
};

}
