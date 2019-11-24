#pragma once
#include <core/allocator.hxx>
#include <vulkan/vulkan.h>

namespace render::vulkan
{

    class VulkanAttachment
    {
    public:
        VulkanAttachment() noexcept;
        ~VulkanAttachment() noexcept;
    };

} // namespace render::vulkan
