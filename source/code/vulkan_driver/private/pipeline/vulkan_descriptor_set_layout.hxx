#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <core/pod/array.hxx>
#include <vulkan/vulkan.h>

namespace render::vulkan
{

    class VulkanDescriptorSetLayout
    {
    public:
        VulkanDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptor_set_layout, core::pod::Array<VkDescriptorSetLayoutBinding> binding) noexcept;
        ~VulkanDescriptorSetLayout() noexcept;

        auto native_handle() const noexcept -> VkDescriptorSetLayout { return _native_handle; }

        auto layout_bindings() const noexcept -> core::pod::Array<VkDescriptorSetLayoutBinding> const& { return _layout_bindings; }

    private:
        VkDevice _device_handle;
        VkDescriptorSetLayout _native_handle;
        core::pod::Array<VkDescriptorSetLayoutBinding> _layout_bindings;
    };

    auto create_descriptor_set_layout(
        core::allocator& alloc,
        VkDevice device,
        core::pod::Array<VkDescriptorSetLayoutBinding> const& layout_bindings) noexcept -> core::memory::unique_pointer<VulkanDescriptorSetLayout>;

} // namespace render::vulkan
