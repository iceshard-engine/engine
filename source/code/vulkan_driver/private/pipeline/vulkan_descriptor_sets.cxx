#include "vulkan_descriptor_sets.hxx"
#include <core/pod/hash.hxx>

namespace render::vulkan
{

    VulkanDescriptorSets::VulkanDescriptorSets(VkDevice device, VkDescriptorPool descriptor_pool, core::pod::Array<VkDescriptorSet> descriptor_sets) noexcept
        : _device_handle{ device }
        , _pool_handle{ descriptor_pool }
        , _native_handles{ descriptor_sets }
    {
    }

    VulkanDescriptorSets::~VulkanDescriptorSets() noexcept
    {
        vkDestroyDescriptorPool(_device_handle, _pool_handle, nullptr);
    }

    void VulkanDescriptorSets::write_descriptor_set(
        uint32_t descriptor_set_index,
        VkDescriptorType type,
        VkDescriptorBufferInfo const& buffer_info) noexcept
    {
        VkWriteDescriptorSet write_info = {};
        write_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_info.pNext = nullptr;
        write_info.dstSet = _native_handles[descriptor_set_index];
        write_info.descriptorCount = 1;
        write_info.descriptorType = type;
        write_info.pBufferInfo = &buffer_info;
        write_info.dstArrayElement = 0;
        write_info.dstBinding = 0;

        vkUpdateDescriptorSets(_device_handle, 1, &write_info, 0, nullptr);
    }

    auto create_vulkan_descriptor_sets(
        core::allocator& alloc,
        VkDevice device,
        core::pod::Array<VulkanDescriptorSetLayout*> const& layouts) noexcept -> core::memory::unique_pointer<VulkanDescriptorSets>
    {
        core::pod::Hash<VkDescriptorPoolSize> pool_sizes{ alloc };
        core::pod::Array<VkDescriptorSetLayout> descriptor_set_layouts{ alloc };

        for (auto const& layout : layouts)
        {
            core::pod::array::push_back(descriptor_set_layouts, layout->native_handle());
            for (auto const& layut_binding : layout->layout_bindings())
            {
                // clang-format off
                VkDescriptorPoolSize descriptor_pool_size = core::pod::hash::get(
                    pool_sizes,
                    layut_binding.descriptorType,
                    VkDescriptorPoolSize{
                        .type = layut_binding.descriptorType,
                        .descriptorCount = 0
                    }
                );
                // clang-format on

                descriptor_pool_size.descriptorCount += 1;
                core::pod::hash::set(pool_sizes, layut_binding.descriptorType, std::move(descriptor_pool_size));
            }
        }

        IS_ASSERT(core::pod::array::any(pool_sizes._data), "No layout bindings where provided!");

        core::pod::Array<VkDescriptorPoolSize> pool_sizes_array{ alloc };
        for (auto const& pool_size : pool_sizes)
        {
            core::pod::array::push_back(pool_sizes_array, pool_size.value);
        }

        VkDescriptorPoolCreateInfo descriptor_pool_info = {};
        descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptor_pool_info.pNext = nullptr;
        descriptor_pool_info.maxSets = core::pod::array::size(pool_sizes_array);
        descriptor_pool_info.poolSizeCount = core::pod::array::size(pool_sizes_array);
        descriptor_pool_info.pPoolSizes = core::pod::array::begin(pool_sizes_array);

        VkDescriptorPool descriptor_pool;
        auto api_result = vkCreateDescriptorPool(device, &descriptor_pool_info, nullptr, &descriptor_pool);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create descriptor pool!");

        VkDescriptorSetAllocateInfo alloc_info;
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;
        alloc_info.descriptorPool = descriptor_pool;
        alloc_info.descriptorSetCount = core::pod::array::size(descriptor_set_layouts);
        alloc_info.pSetLayouts = core::pod::array::begin(descriptor_set_layouts);

        core::pod::Array<VkDescriptorSet> descriptor_sets{ alloc };
        core::pod::array::resize(descriptor_sets, core::pod::array::size(descriptor_set_layouts));

        api_result = vkAllocateDescriptorSets(device, &alloc_info, core::pod::array::begin(descriptor_sets));
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't create descriptor sets!");

        return core::memory::make_unique<VulkanDescriptorSets>(alloc, device, descriptor_pool, std::move(descriptor_sets));
    }

} // namespace render::vulkan
