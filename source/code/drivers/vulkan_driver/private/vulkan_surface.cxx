#include <core/debug/assert.hxx>

#include "vulkan_surface.hxx"
#include <SDL.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <SDL_syswm.h>
#include <iceshard\renderer\vulkan\vulkan_surface.hxx>

namespace render::vulkan
{

    VulkanSurface::VulkanSurface(VkSurfaceKHR handle) noexcept
        : _surface_handle{ handle }
    {
    }

    namespace win32
    {

        class VulkanWin32Surface : public VulkanSurface
        {
        public:
            VulkanWin32Surface(SDL_Window* sdl_window, VkInstance instance, VkSurfaceKHR surface) noexcept
                : VulkanSurface{ surface }
                , _render_window{ sdl_window }
                , _instance_handle{ instance }
            {
            }

            ~VulkanWin32Surface() noexcept
            {
                vkDestroySurfaceKHR(_instance_handle, _surface_handle, nullptr);
                SDL_DestroyWindow(_render_window);
            }

        private:
            SDL_Window* _render_window;

            VkInstance _instance_handle;
        };

    } // namespace win32

    auto create_surface(core::allocator& alloc, VkInstance vulkan_instance) noexcept -> core::memory::unique_pointer<VulkanSurface>
    {
        const bool sdl2_init_video = SDL_InitSubSystem(SDL_INIT_VIDEO) == 0;
        IS_ASSERT(sdl2_init_video == true, "Initialization od SDL2 Video subsystem failed! Error: '{}'", SDL_GetError());

        SDL_Window* sdl_window = SDL_CreateWindow(
            "IceShard - Vulkan Win32 Window",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            1280, 720,
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);

        SDL_SysWMinfo wm_info{};
        SDL_GetWindowWMInfo(sdl_window, &wm_info);

        VkWin32SurfaceCreateInfoKHR surface_create_info = {};
        surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surface_create_info.pNext = NULL;
        surface_create_info.hinstance = wm_info.info.win.hinstance;
        surface_create_info.hwnd = wm_info.info.win.window;

        VkSurfaceKHR vulkan_surface;
        auto api_result = vkCreateWin32SurfaceKHR(vulkan_instance, &surface_create_info, nullptr, &vulkan_surface);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Failed to create Vulkan surface!");

        return core::memory::make_unique<VulkanSurface, win32::VulkanWin32Surface>(alloc, sdl_window, vulkan_instance, vulkan_surface);
    }

} // namespace render::vulkan
