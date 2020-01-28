#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <core/pod/array.hxx>
#include <core/collections.hxx>

#include <iceshard/renderer/vulkan/vulkan_resource_layouts.hxx>

#include "vulkan_descriptor_pool.hxx"
#include "vulkan_descriptor_set_layout.hxx"

namespace render::vulkan
{

    class VulkanDescriptorSets
    {
    public:
        VulkanDescriptorSets(VkDevice device, VkPipelineLayout pipeline_layout, VkDescriptorPool descriptor_pool, core::pod::Array<VkDescriptorSet> descriptor_sets) noexcept;
        ~VulkanDescriptorSets() noexcept;

        auto pipeline_layout() const noexcept -> VkPipelineLayout { return _pipeline_layout; }

        auto native_handles() const noexcept -> core::pod::Array<VkDescriptorSet> const& { return _native_handles; }

        void native_handles(core::pod::Array<VkDescriptorSet>& sets) const noexcept { sets = _native_handles; }

        void write_descriptor_set(
            uint32_t descriptor_set_index,
            uint32_t binding,
            VkDescriptorType type,
            VkDescriptorBufferInfo const& buffer_info
        ) noexcept;

        void write_descriptor_set(
            uint32_t descriptor_set_index,
            uint32_t binding,
            VkDescriptorType type,
            VkDescriptorImageInfo const& image_info
        ) noexcept;

    private:
        VkDevice _device_handle;
        VkDescriptorPool _pool_handle;
        VkPipelineLayout _pipeline_layout;
        core::pod::Array<VkDescriptorSet> _native_handles;
    };

    auto create_vulkan_descriptor_sets(
        core::allocator& alloc,
        VkPipelineLayout pipeline_layout,
        vulkan::VulkanDescriptorPool& descriptor_pool,
        iceshard::renderer::vulkan::VulkanResourceLayouts const& layouts
    ) noexcept -> core::memory::unique_pointer<VulkanDescriptorSets>;

} // namespace render::vulkan
