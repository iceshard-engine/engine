/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_pass.hxx>
#include <ice/pod/array.hxx>

#include "vk_include.hxx"

namespace ice::render::vk
{

    class VulkanRenderPass final
    {
    public:
        VulkanRenderPass(
            VkRenderPass vk_renderpass,
            VkDevice vk_device
        ) noexcept;
        ~VulkanRenderPass() noexcept = default;

    private:
        VkRenderPass _vk_renderpass;
        VkDevice _vk_device;
    };

} // namespace ice::render::vk
