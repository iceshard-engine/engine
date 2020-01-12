#include "vulkan_device_memory_manager.hxx"
#include <core/memory.hxx>

namespace render::vulkan
{

    namespace detail
    {

        auto allocate_device_memory(VkDevice graphics_device, VkDeviceSize size, uint32_t type_index) noexcept -> VkDeviceMemory
        {
            VkMemoryAllocateInfo alloc_info = {};
            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.pNext = nullptr;
            alloc_info.allocationSize = size;
            alloc_info.memoryTypeIndex = type_index;

            VkDeviceMemory buffer_memory_handle;
            auto api_result = vkAllocateMemory(graphics_device, &alloc_info, nullptr, &buffer_memory_handle);
            IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't allocate memory for depth image!");

            return buffer_memory_handle;
        }

        auto align_block_offset(uint32_t value, uint32_t const alignment) noexcept
        {
            auto const alignment_missmatch = value % alignment;
            if (alignment_missmatch != 0)
            {
                value += alignment - alignment_missmatch;
            }
            return value;
        }

    } // namespace detail


    VulkanDeviceMemoryManager::VulkanDeviceMemoryManager(core::allocator& alloc, VulkanPhysicalDevice const* physical_device, VkDevice graphics_device) noexcept
        : _physical_device{ physical_device }
        , _graphics_device{ graphics_device }
        , _buffer_allocators{ alloc }
    {
    }

    VulkanDeviceMemoryManager::~VulkanDeviceMemoryManager() noexcept
    {
        for (auto const& blocks : _buffer_allocators)
        {
            vkFreeMemory(_graphics_device, blocks.second.device_memory_handle, nullptr);
        }
    }

    bool VulkanDeviceMemoryManager::allocate_memory(VkBuffer buffer, VulkanMemoryInfo& memory_info) noexcept
    {
        VkMemoryRequirements memory_requirements;
        vkGetBufferMemoryRequirements(_graphics_device, buffer, &memory_requirements);

        uint32_t memory_type_index = 0;
        bool memory_type_found = _physical_device->find_memory_type_index(
            memory_requirements,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            memory_type_index
        );
        IS_ASSERT(memory_type_found, "No memory type found for requested buffer!");

        if (_buffer_allocators.count(memory_type_index) == 0)
        {
            uint32_t requested_block_size = 1024 * 1024 * 10;
            DeviceMemoryBlock memory_entry{
                .device_memory_handle = detail::allocate_device_memory(_graphics_device, requested_block_size, memory_type_index),
                .device_memory_total = requested_block_size,
                .device_memory_usage = 0,
            };

            _buffer_allocators.emplace(
                memory_type_index,
                std::move(memory_entry)
            );
        }

        auto& memory_block = _buffer_allocators.at(memory_type_index);
        auto memory_block_offset = detail::align_block_offset(memory_block.device_memory_usage, static_cast<uint32_t>(memory_requirements.alignment));
        IS_ASSERT(memory_block_offset + memory_requirements.size <= memory_block.device_memory_total, "Memory block for type {} is insufficient!", memory_type_index);

        memory_block.device_memory_usage = static_cast<uint32_t>(memory_block_offset + memory_requirements.size);

        auto api_result = vkBindBufferMemory(_graphics_device, buffer, memory_block.device_memory_handle, memory_block_offset);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't bind memory to buffer!");

        memory_info.memory_handle = memory_block.device_memory_handle;
        memory_info.memory_offset = memory_block_offset;
        memory_info.memory_size = static_cast<uint32_t>(memory_requirements.size);

        return api_result == VkResult::VK_SUCCESS;
    }

} // namespace render::vulkan
