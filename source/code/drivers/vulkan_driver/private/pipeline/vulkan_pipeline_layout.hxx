#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <core/pod/array.hxx>
#include <core/collections.hxx>

#include <iceshard/renderer/vulkan/vulkan_resource_layouts.hxx>

namespace render::vulkan
{

    class VulkanPipelineLayout
    {
    public:
        VulkanPipelineLayout(VkDevice device, VkPipelineLayout pipeline_layout) noexcept;
        ~VulkanPipelineLayout() noexcept;

        auto native_handle() const noexcept -> VkPipelineLayout { return _native_handle; }

    private:
        VkDevice _device_handle;
        VkPipelineLayout _native_handle;
    };

    class VulkanDescriptorSetLayout;

    auto create_pipeline_layout(
        core::allocator& alloc,
        VkDevice device,
        iceshard::renderer::vulkan::VulkanResourceLayouts const& reosurce_layouts
    ) noexcept -> core::memory::unique_pointer<VulkanPipelineLayout>;

} // namespace render::vulkan
