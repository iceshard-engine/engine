#include "vulkan_descriptor_sets.hxx"
#include <core/pod/hash.hxx>
#include <core/collections.hxx>

namespace render::vulkan
{

    VulkanDescriptorSets::VulkanDescriptorSets(
        VkDevice device,
        VkDescriptorPool descriptor_pool,
        core::pod::Array<VkDescriptorSet> descriptor_sets
    ) noexcept
        : _device_handle{ device }
        , _pool_handle{ descriptor_pool }
        , _native_handles{ descriptor_sets }
    {
    }

    VulkanDescriptorSets::~VulkanDescriptorSets() noexcept
    {
        vkFreeDescriptorSets(
            _device_handle,
            _pool_handle,
            core::pod::array::size(_native_handles),
            core::pod::array::begin(_native_handles)
        );
    }

    void VulkanDescriptorSets::write_descriptor_set(
        uint32_t descriptor_set_index,
        uint32_t binding,
        VkDescriptorType type,
        VkDescriptorBufferInfo const& buffer_info
    ) noexcept
    {
        VkWriteDescriptorSet write_info = {};
        write_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_info.pNext = nullptr;
        write_info.dstSet = _native_handles[descriptor_set_index];
        write_info.dstArrayElement = 0;
        write_info.dstBinding = binding;
        write_info.descriptorCount = 1;
        write_info.descriptorType = type;
        write_info.pBufferInfo = &buffer_info;

        vkUpdateDescriptorSets(_device_handle, 1, &write_info, 0, nullptr);
    }

    void VulkanDescriptorSets::write_descriptor_set(
        uint32_t descriptor_set_index,
        uint32_t binding,
        VkDescriptorType type,
        VkDescriptorImageInfo const& image_info
    ) noexcept
    {
        VkWriteDescriptorSet write_info = {};
        write_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_info.pNext = nullptr;
        write_info.dstSet = _native_handles[descriptor_set_index];
        write_info.dstArrayElement = 0;
        write_info.dstBinding = binding;
        write_info.descriptorCount = 1;
        write_info.descriptorType = type;
        write_info.pImageInfo = &image_info;

        vkUpdateDescriptorSets(_device_handle, 1, &write_info, 0, nullptr);
    }

    auto create_vulkan_descriptor_sets(
        core::allocator& alloc,
        render::vulkan::VulkanDescriptorPool& descriptor_pool,
        core::Vector<core::memory::unique_pointer<VulkanDescriptorSetLayout>> const& layouts
    ) noexcept -> core::memory::unique_pointer<VulkanDescriptorSets>
    {
        core::pod::Array<VkDescriptorSetLayout> descriptor_set_layouts{ alloc };

        for (auto const& layout : layouts)
        {
            core::pod::array::push_back(descriptor_set_layouts, layout->native_handle());
        }

        VkDescriptorSetAllocateInfo alloc_info;
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = descriptor_pool.native_handle();
        alloc_info.descriptorSetCount = core::pod::array::size(descriptor_set_layouts);
        alloc_info.pSetLayouts = core::pod::array::begin(descriptor_set_layouts);

        core::pod::Array<VkDescriptorSet> descriptor_sets{ alloc };
        core::pod::array::resize(descriptor_sets, core::pod::array::size(descriptor_set_layouts));

        auto api_result = vkAllocateDescriptorSets(descriptor_pool.graphics_device(), &alloc_info, core::pod::array::begin(descriptor_sets));
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create descriptor sets!");

        return core::memory::make_unique<VulkanDescriptorSets>(alloc,
            descriptor_pool.graphics_device(),
            descriptor_pool.native_handle(),
            std::move(descriptor_sets)
        );
    }

} // namespace render::vulkan
