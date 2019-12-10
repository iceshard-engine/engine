#pragma once
#include <render_system/render_api.hxx>
#include <vulkan/vulkan.h>

namespace render::api::v1::vulkan
{

    void init_api(void* ptr) noexcept;

    static_assert(sizeof(VkCommandBuffer) == sizeof(CommandBuffer), "Command buffer handle differs in size!");

    auto native(CommandBuffer command_buffer) noexcept -> VkCommandBuffer;

} // namespace render::api::v1::vulkan
