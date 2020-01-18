#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <vulkan/vulkan.h>

namespace render::vulkan
{

    class VulkanSurface
    {
    public:
        VulkanSurface(VkSurfaceKHR handle) noexcept;
        virtual ~VulkanSurface() noexcept = default;

        auto native_handle() noexcept -> VkSurfaceKHR { return _surface_handle; }

    protected:
        VkSurfaceKHR _surface_handle;
    };

    auto create_surface(core::allocator& alloc, VkInstance vulkan_instance) noexcept -> core::memory::unique_pointer<VulkanSurface>;

} // namespace render::vulkan
