#pragma once
#include <iceshard/renderer/vulkan/vulkan_surface.hxx>
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>

#include <SDL.h>

namespace iceshard::renderer::vulkan::sdl2
{

    class SDL2_VulkanSurface final
    {
    public:
        SDL2_VulkanSurface(SDL_Window* window, VkInstance vk_instance, VkSurfaceKHR vk_surface) noexcept;
        ~SDL2_VulkanSurface() noexcept;

        auto native_handle() noexcept
        {
            return _vulkan_surface;
        }

    private:
        SDL_Window* _window;
        VkInstance _vulkan_instance;
        VkSurfaceKHR _vulkan_surface;
    };

    union VulkanSurface_SDL2
    {
        VulkanSurface surface = VulkanSurface::Invalid;
        SDL2_VulkanSurface* sdl2_surface;
    };

} // namespace iceshard::renderer::vulkan
