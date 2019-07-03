#pragma once

namespace mooned::io
{

enum class RenderBackend
{
    // The window doesn't have any render back-end associated with it
    NONE,
    // The new Vulcan API (available on Windows and Linux, not available yet)
    VULKAN,
    // OpenGL, version 4.5 (available on Windows and Linux, not available yet on Linux)
    OPENGL,
    // DirectX, version 12 (available on Windows, not available yet)
    DIRECTX,
    // Metal, version 2 (available on MacOS, not available yet)
    METAL
};

class IOSystem;

struct Window;

Window* render_window(IOSystem* system);

const char* window_title(Window* window);

void set_window_title(Window* window, const char* title);

void window_position(Window* window, int& x, int &y);

void window_drawable_size(Window* window, int& width, int& height);

void window_size(Window* window, int& width, int& height);

bool window_has_focus(Window* window);

void window_show(Window* window);

void window_hide(Window* window);

void* window_handle(Window* window);

void* window_native_handle(Window* window);

}
