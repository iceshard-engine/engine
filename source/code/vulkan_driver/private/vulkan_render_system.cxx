#include <core/memory.hxx>
#include <core/pod/array.hxx>
#include <core/message/buffer.hxx>
#include <core/message/operations.hxx>

#include <render_system/render_system.hxx>
#include <render_system/render_commands.hxx>

#include <SDL.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

namespace render
{

    class OpenGlRenderSystem : public render::RenderSystem
    {
    public:
        OpenGlRenderSystem(core::allocator& alloc) noexcept
            : render::RenderSystem{ }
            , _render_window{ nullptr }
            , _command_buffer{ alloc }
        {
            const bool sdl2_init_video = SDL_InitSubSystem(SDL_INIT_VIDEO) == 0;
            if (sdl2_init_video == false)
            {
                IS_ASSERT(sdl2_init_video == true, "Initialization od SDL2 Video subsystem failed! Error: '{}'", SDL_GetError());
            }


            _render_window = SDL_CreateWindow(
                "IceShard - Test window",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                1280, 720,
                SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN
            );
        }

        auto command_buffer() noexcept -> render::RenderCommandBuffer&
        {
            return _command_buffer;
        }

        void swap() noexcept
        {
            _command_buffer.visit([](const render::CommandName& name, core::data_view data)
                {
                    if (name.identifier == render::data::Clear::command_name.identifier)
                    {
                        IS_ASSERT(data.size() == sizeof(render::data::Clear), "Data size does not match expected type size.");

                        [[maybe_unused]]
                        auto* clear_data = reinterpret_cast<const render::data::Clear*>(data.data());

                    }
                });
        }

        ~OpenGlRenderSystem() noexcept override
        {
            SDL_DestroyWindow(_render_window);
        }

    private:
        SDL_Window* _render_window;

        render::RenderCommandBuffer _command_buffer;
    };

} // render


extern "C"
{
    __declspec(dllexport) auto create_render_system(core::allocator& alloc) -> render::RenderSystem*
    {
        return alloc.make<render::OpenGlRenderSystem>(alloc);
    }

    __declspec(dllexport) void release_render_system(core::allocator& alloc, render::RenderSystem* driver)
    {
        alloc.destroy(driver);
    }
}
