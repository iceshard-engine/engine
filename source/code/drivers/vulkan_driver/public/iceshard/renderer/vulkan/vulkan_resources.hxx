#pragma once
#include <iceshard/renderer/render_resources.hxx>
#include <iceshard/renderer/vulkan/vulkan_resource_layouts.hxx>
#include <core/cexpr/stringid.hxx>
#include <core/pod/array.hxx>

namespace iceshard::renderer::vulkan
{

    struct VulkanResourceSet
    {
        core::stringid_type name;

        VkDescriptorSet uniform_set;
        VkDescriptorSet texture_set;
    };

    void create_resource_set(
        core::stringid_arg_type name,
        core::pod::Array<RenderResource> const& resources,
        VulkanResourceLayouts const& resource_layouts,
        VulkanResourceSet& set
    ) noexcept;

} // namespace iceshard::renderer::vulkan
