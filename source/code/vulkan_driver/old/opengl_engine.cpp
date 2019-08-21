#include "opengl_engine.h"

#include <renderlib/render_vertex_array.h>
#include <iolib_sdl2/window.h>

#include <SDL.h>
#include <GL/glew.h>

#include <cassert>

void mooned::render::initialize_window(mooned::io::Window* window, mooned::io::RenderBackend backend)
{
    // Set the OpenGL render context attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // Sets the render backed using the SDL2 io backend.
    mooned::io::set_window_render_backend(window, backend);
}

mooned::render::opengl::OpenGlRenderEngine::OpenGlRenderEngine()
{
    // Set the OpenGL context settings here
}

mooned::render::opengl::OpenGlRenderEngine::~OpenGlRenderEngine()
{
}

void mooned::render::opengl::OpenGlRenderEngine::initialize(mooned::io::Window* window)
{
    assert(mooned::io::window_render_backend(window) == mooned::io::RenderBackend::OPENGL);

    _render_window = window;

    // Get the SDL window created
    _sdl_window = reinterpret_cast<SDL_Window*>(mooned::io::window_handle(_render_window));

    // Create the OpenGL context
    SDL_GL_CreateContext(_sdl_window);

    // Initialize glew
    glewExperimental = true;
    bool success = glewInit() == GLEW_OK;
    assert(success);
}

mooned::io::RenderBackend mooned::render::opengl::OpenGlRenderEngine::backend() const
{
    return mooned::io::RenderBackend::OPENGL;
}

mooned::render::RenderEngineVersion mooned::render::opengl::OpenGlRenderEngine::version() const
{
    return RenderEngineVersion{ 4500 };
}

void mooned::render::opengl::OpenGlRenderEngine::swap()
{
    SDL_GL_SwapWindow(_sdl_window);
}

