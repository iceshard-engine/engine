#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <core/pod/array.hxx>
#include <vulkan/vulkan.h>

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

    auto create_pipeline_layout(
        core::allocator& alloc,
        VkDevice device,
        core::pod::Array<VkDescriptorSetLayout> const& descriptor_sets) noexcept -> core::memory::unique_pointer<VulkanPipelineLayout>;

} // namespace render::vulkan
