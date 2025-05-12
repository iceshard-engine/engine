/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_fence.hxx>
#include "vk_include.hxx"

namespace ice::render::vk
{

    class VulkanFence : public ice::render::RenderFence
    {
    public:
        VulkanFence(VkDevice vk_device, VkFence vk_fence) noexcept;
        ~VulkanFence() noexcept;

        auto native() const noexcept -> VkFence;

        bool wait(ice::u64 timeout_ns) noexcept override;
        void reset() noexcept override;

    private:
        VkDevice _vk_device;
        VkFence _vk_fence;
    };

} // namespace ice::render
