#pragma once
#include <renderlib/render_engine.h>

struct SDL_Window;

namespace mooned::render::opengl
{

class OpenGlRenderEngine : public mooned::render::RenderEngine
{
public:
    OpenGlRenderEngine();
    ~OpenGlRenderEngine() override;

    void initialize(mooned::io::Window* window) override;

    void execute(const CommandBuffer& command_buffer) override;

    void swap() override;

    mooned::io::RenderBackend backend() const override;
    RenderEngineVersion version() const override;

private:
    mooned::io::Window* _render_window;
    SDL_Window* _sdl_window;
};

}
