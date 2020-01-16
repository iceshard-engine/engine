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

        auto align_block_offset(VkDeviceSize value, VkDeviceSize const alignment) noexcept
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
        , _memory_blocks{ alloc }
        , _current_memory_blocks{ alloc }
    {
    }

    VulkanDeviceMemoryManager::~VulkanDeviceMemoryManager() noexcept
    {
        for (auto const& block : _memory_blocks)
        {
            vkFreeMemory(_graphics_device, block.device_memory_handle, nullptr);
        }
    }

    bool VulkanDeviceMemoryManager::allocate_memory(VkBuffer buffer, VkMemoryPropertyFlags flags, VulkanMemoryInfo& memory_info) noexcept
    {
        VkMemoryRequirements memory_requirements;
        vkGetBufferMemoryRequirements(_graphics_device, buffer, &memory_requirements);

        uint32_t memory_type_index = 0;
        bool memory_type_found = _physical_device->find_memory_type_index(
            memory_requirements,
            flags,
            memory_type_index
        );
        IS_ASSERT(memory_type_found, "No memory type found for requested buffer!");

        VulkanMemoryInfo temp_memory_info;
        allocate_memory(memory_type_index, memory_requirements.size, memory_requirements.alignment, temp_memory_info);

        auto api_result = vkBindBufferMemory(_graphics_device, buffer, temp_memory_info.memory_handle, temp_memory_info.memory_offset);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't bind memory to buffer!");

        memory_info = temp_memory_info;
        return api_result == VkResult::VK_SUCCESS;
    }

    bool VulkanDeviceMemoryManager::allocate_memory(VkImage image, VkMemoryPropertyFlags flags, VulkanMemoryInfo& memory_info) noexcept
    {
        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(_graphics_device, image, &memory_requirements);

        uint32_t memory_type_index = 0;
        bool memory_type_found = _physical_device->find_memory_type_index(
            memory_requirements,
            flags,
            memory_type_index
        );
        IS_ASSERT(memory_type_found, "No memory type found for requested buffer!");

        VulkanMemoryInfo temp_memory_info;
        allocate_memory(memory_type_index, memory_requirements.size, memory_requirements.alignment, temp_memory_info);

        auto api_result = vkBindImageMemory(_graphics_device, image, temp_memory_info.memory_handle, temp_memory_info.memory_offset);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't bind memory to buffer!");

        memory_info = temp_memory_info;
        return api_result == VkResult::VK_SUCCESS;
    }

    void VulkanDeviceMemoryManager::allocate_memory(uint32_t memory_type, VkDeviceSize size, VkDeviceSize alignment, VulkanMemoryInfo& memory_info) noexcept
    {
        static constexpr VkDeviceSize block_size = 1024 * 1024 * 4;

        if (block_size <= size)
        {
            DeviceMemoryBlock memory_block {
                .device_memory_handle = detail::allocate_device_memory(_graphics_device, size, memory_type),
                .device_memory_total = size,
                .device_memory_usage = size,
            };

            memory_info.memory_handle = memory_block.device_memory_handle;
            memory_info.memory_offset = 0;
            memory_info.memory_size = static_cast<uint32_t>(size);

            _memory_blocks.emplace_back(std::move(memory_block));
        }
        else
        {
            if (_current_memory_blocks.count(memory_type) == 0)
            {
                _memory_blocks.emplace_back(DeviceMemoryBlock{
                    .device_memory_handle = detail::allocate_device_memory(_graphics_device, block_size, memory_type),
                    .device_memory_total = block_size,
                    .device_memory_usage = 0,
                });

                _current_memory_blocks.emplace(
                    memory_type,
                    &_memory_blocks.back()
                );
            }

            DeviceMemoryBlock* memory_block = _current_memory_blocks.at(memory_type);

            auto memory_block_offset = detail::align_block_offset(memory_block->device_memory_usage, static_cast<uint32_t>(alignment));
            if (memory_block_offset + size >= memory_block->device_memory_total)
            {
                _memory_blocks.emplace_back(DeviceMemoryBlock{
                    .device_memory_handle = detail::allocate_device_memory(_graphics_device, block_size, memory_type),
                    .device_memory_total = block_size,
                    .device_memory_usage = 0,
                });

                _current_memory_blocks[memory_type] = &_memory_blocks.back();
            }

            memory_block = _current_memory_blocks.at(memory_type);
            memory_block_offset = detail::align_block_offset(memory_block->device_memory_usage, static_cast<uint32_t>(alignment));
            IS_ASSERT(memory_block_offset + size <= memory_block->device_memory_total, "Memory block for type {} is insufficient!", memory_type);

            memory_block->device_memory_usage = static_cast<uint32_t>(memory_block_offset + size);

            memory_info.memory_handle = memory_block->device_memory_handle;
            memory_info.memory_offset = static_cast<uint32_t>(memory_block_offset);
            memory_info.memory_size = static_cast<uint32_t>(size);
        }
    }

} // namespace render::vulkan
