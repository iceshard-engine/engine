#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <core/pod/array.hxx>
#include <vulkan/vulkan.h>

#include "vulkan_descriptor_set_layout.hxx"

namespace render::vulkan
{

    class VulkanDescriptorSets
    {
    public:
        VulkanDescriptorSets(VkDevice device, VkDescriptorPool descriptor_pool, core::pod::Array<VkDescriptorSet> descriptor_sets) noexcept;
        ~VulkanDescriptorSets() noexcept;

        void write_descriptor_set(
            uint32_t descriptor_set_index,
            VkDescriptorType type,
            VkDescriptorBufferInfo const& buffer_info) noexcept;

    private:
        VkDevice _device_handle;
        VkDescriptorPool _pool_handle;
        core::pod::Array<VkDescriptorSet> _native_handles;
    };

    auto create_vulkan_descriptor_sets(
        core::allocator& alloc,
        VkDevice device,
        core::pod::Array<VulkanDescriptorSetLayout*> const& layouts) noexcept -> core::memory::unique_pointer<VulkanDescriptorSets>;

} // namespace render::vulkan
