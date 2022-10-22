/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_pipeline.hxx>
#include "vk_include.hxx"

namespace ice::render::vk
{

    class VulkanPipelineLayout final
    {
    public:
        VulkanPipelineLayout() noexcept;
        ~VulkanPipelineLayout() noexcept;

    private:
        VkPipelineLayout _vk_piepline_layout;
    };

} // namespace ice::render::vk
