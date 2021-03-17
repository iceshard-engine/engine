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
