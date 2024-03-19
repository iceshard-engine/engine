#pragma once
#include <ice/render/render_command_buffer.hxx>
#include <ice/profiler.hxx>

#include "vk_include.hxx"
#include "vk_render_profiler.hxx"

namespace ice::render::vk
{

    struct VulkanCommandBuffer : ice::render::detail::vk::VulkanProfiledCommandBuffer
    {
        VkCommandBuffer buffer;

        static auto handle(VulkanCommandBuffer* native) noexcept
        {
            return static_cast<ice::render::CommandBuffer>(reinterpret_cast<uintptr_t>(native));
        }

        static auto native(ice::render::CommandBuffer handle) noexcept
        {
            return reinterpret_cast<VulkanCommandBuffer*>(static_cast<uintptr_t>(handle));
        }
    };

} // namespace ice::render::vk
