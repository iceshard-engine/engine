#include "vulkan_surface.sdl2.hxx"
#include <core/debug/assert.hxx>

#include <SDL_syswm.h>

namespace iceshard::renderer::vulkan
{

    sdl2::SDL2_VulkanSurface::SDL2_VulkanSurface(SDL_Window* window, VkInstance vk_instance, VkSurfaceKHR vk_surface) noexcept
        : _window{ window }
        , _vulkan_instance{ vk_instance }
        , _vulkan_surface{ vk_surface }
    {
    }

    sdl2::SDL2_VulkanSurface::~SDL2_VulkanSurface() noexcept
    {
        vkDestroySurfaceKHR(_vulkan_instance, _vulkan_surface, nullptr);
    }

    auto create_surface(core::allocator& alloc, VkInstance vulkan_instance, VkExtent2D initial_extent) noexcept -> VulkanSurface
    {
        sdl2::VulkanSurface_SDL2 sdl2_surface{ };

        const bool sdl2_init_video = SDL_InitSubSystem(SDL_INIT_VIDEO) == 0;
        IS_ASSERT(sdl2_init_video == true, "Initialization od SDL2 Video subsystem failed! Error: '{}'", SDL_GetError());

        SDL_Window* sdl_window = SDL_CreateWindow(
            "IceShard - Vulkan Win32 Window",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            initial_extent.width, initial_extent.height,
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN
        );

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

        sdl2_surface.sdl2_surface = alloc.make<sdl2::SDL2_VulkanSurface>(sdl_window, vulkan_instance, vulkan_surface);
        return sdl2_surface.surface;
    }

    void destroy_surface(core::allocator& alloc, VulkanSurface surface) noexcept
    {
        sdl2::VulkanSurface_SDL2 sdl2_surface{ surface };

        alloc.destroy(sdl2_surface.sdl2_surface);
    }

} // namespace iceshard::renderer::vulkan
