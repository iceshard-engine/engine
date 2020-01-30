#include "vulkan_device_memory_manager.hxx"
#include <core/memory.hxx>
#include <core/pod/hash.hxx>
#include <core/allocators/stack_allocator.hxx>

namespace win32
{
}

namespace iceshard::renderer::vulkan
{

    namespace detail
    {

        auto allocate_device_memory(VkDevice graphics_device, VkDeviceSize size, uint32_t type_index, VkAllocationCallbacks const* callbacks) noexcept -> VkDeviceMemory
        {
            VkMemoryAllocateInfo alloc_info = {};
            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.pNext = nullptr;
            alloc_info.allocationSize = size;
            alloc_info.memoryTypeIndex = type_index;

            VkDeviceMemory buffer_memory_handle;
            auto api_result = vkAllocateMemory(graphics_device, &alloc_info, callbacks, &buffer_memory_handle);
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


    VulkanDeviceMemoryManager::VulkanDeviceMemoryManager(core::allocator& alloc, iceshard::renderer::vulkan::VulkanDevices devices) noexcept
        : _devices{ devices }
        , _memory_blocks{ alloc }
        , _block_info{ alloc }
        , _current_memory_blocks{ alloc }
        , _vulkan_allocator{ alloc }
    {
    }

    VulkanDeviceMemoryManager::~VulkanDeviceMemoryManager() noexcept
    {
        for (auto const& block : _memory_blocks)
        {
            vkFreeMemory(_devices.graphics.handle, block.device_memory_handle, _vulkan_allocator.vulkan_callbacks());
        }
    }

    bool VulkanDeviceMemoryManager::allocate_memory(VkBuffer buffer, VkMemoryPropertyFlags memory_property_flags, VulkanMemoryInfo& memory_info) noexcept
    {
        VkMemoryRequirements memory_requirements;
        vkGetBufferMemoryRequirements(_devices.graphics.handle, buffer, &memory_requirements);

        uint32_t memory_type_index = 0;
        bool memory_type_found = iceshard::renderer::vulkan::find_memory_type_index(
            _devices,
            memory_requirements,
            memory_property_flags,
            memory_type_index
        );
        IS_ASSERT(memory_type_found, "No memory type found for requested buffer!");

        VulkanMemoryInfo temp_memory_info;
        allocate_memory(memory_type_index, memory_requirements.size, memory_requirements.alignment, temp_memory_info);

        auto api_result = vkBindBufferMemory(_devices.graphics.handle, buffer, temp_memory_info.memory_handle, temp_memory_info.memory_offset);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't bind memory to buffer!");

        memory_info = temp_memory_info;
        return api_result == VkResult::VK_SUCCESS;
    }

    bool VulkanDeviceMemoryManager::allocate_memory(VkImage image, VkMemoryPropertyFlags memory_property_flags, VulkanMemoryInfo& memory_info) noexcept
    {
        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(_devices.graphics.handle, image, &memory_requirements);

        uint32_t memory_type_index = 0;
        bool memory_type_found = iceshard::renderer::vulkan::find_memory_type_index(
            _devices,
            memory_requirements,
            memory_property_flags,
            memory_type_index
        );
        IS_ASSERT(memory_type_found, "No memory type found for requested image!");

        VulkanMemoryInfo temp_memory_info;
        auto block_handle = detail::allocate_device_memory(
            _devices.graphics.handle,
            memory_requirements.size,
            memory_type_index,
            _vulkan_allocator.vulkan_callbacks()
        );
        temp_memory_info.memory_handle = block_handle;
        temp_memory_info.memory_offset = 0;
        temp_memory_info.memory_size = (uint32_t) memory_requirements.size;

        auto api_result = vkBindImageMemory(_devices.graphics.handle, image, temp_memory_info.memory_handle, temp_memory_info.memory_offset);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't bind memory to buffer!");

        memory_info = temp_memory_info;
        return api_result == VkResult::VK_SUCCESS;
    }

    void VulkanDeviceMemoryManager::deallocate_memory(VkBuffer, VulkanMemoryInfo const&) noexcept
    {
        // not implemented
    }

    void VulkanDeviceMemoryManager::deallocate_memory(VkImage, VulkanMemoryInfo const& memory_info) noexcept
    {
        vkFreeMemory(_devices.graphics.handle, memory_info.memory_handle, _vulkan_allocator.vulkan_callbacks());
    }

    void VulkanDeviceMemoryManager::map_memory(VulkanMemoryInfo* ranges, api::DataView* views, uint32_t size) noexcept
    {
        core::memory::stack_allocator_512 temp_alloc;
        core::pod::Hash<void*> mapped_ptrs{ temp_alloc };
        core::pod::hash::reserve(mapped_ptrs, 16);

        for (uint32_t idx = 0; idx < size; ++idx)
        {
            auto const& range = ranges[idx];
            auto const& block_idx = _block_info[range.memory_handle];

            void* mapped_ptr = core::pod::hash::get<void*>(mapped_ptrs, block_idx, nullptr);
            if (nullptr == mapped_ptr)
            {
                auto const& block = _memory_blocks[block_idx];
                auto vk_result = vkMapMemory(
                    _devices.graphics.handle,
                    block.device_memory_handle,
                    0, /* offset always from the beginning */
                    block.device_memory_usage,
                    0, /* empty flags required */
                    &mapped_ptr
                );
                IS_ASSERT(vk_result == VkResult::VK_SUCCESS, "Memory mapping failed!");

                core::pod::hash::set(mapped_ptrs, block_idx, mapped_ptr);
            }

            views[idx].data = core::memory::utils::pointer_add(mapped_ptr, range.memory_offset);
            views[idx].size = range.memory_size;
        }
    }

    void VulkanDeviceMemoryManager::unmap_memory(VulkanMemoryInfo* ranges, uint32_t size)
    {
        core::memory::stack_allocator<256> temp_alloc;
        core::pod::Hash<bool> unmapped_ptrs{ temp_alloc };
        core::pod::hash::reserve(unmapped_ptrs, 16);

        for (uint32_t idx = 0; idx < size; ++idx)
        {
            auto const& range = ranges[idx];
            auto const block_hash = reinterpret_cast<uintptr_t>(range.memory_handle);

            if (core::pod::hash::has(unmapped_ptrs, block_hash) == false)
            {
                vkUnmapMemory(_devices.graphics.handle, range.memory_handle);
                core::pod::hash::set(unmapped_ptrs, block_hash, true);
            }
        }
    }

    void VulkanDeviceMemoryManager::allocate_memory(uint32_t memory_type, VkDeviceSize size, VkDeviceSize alignment, VulkanMemoryInfo& memory_info) noexcept
    {
        static constexpr VkDeviceSize block_size = 1024 * 1024 * 4;

        if (block_size <= size)
        {
            DeviceMemoryBlock memory_block {
                .device_memory_handle = detail::allocate_device_memory(_devices.graphics.handle, size, memory_type, _vulkan_allocator.vulkan_callbacks()),
                .device_memory_total = size,
                .device_memory_usage = size,
            };

            memory_info.memory_handle = memory_block.device_memory_handle;
            memory_info.memory_offset = 0;
            memory_info.memory_size = static_cast<uint32_t>(size);

            _block_info.emplace(memory_info.memory_handle, static_cast<uint32_t>(_memory_blocks.size()));
            _memory_blocks.emplace_back(std::move(memory_block));
        }
        else
        {
            if (_current_memory_blocks.count(memory_type) == 0)
            {
                uint32_t block_index = static_cast<uint32_t>(_memory_blocks.size());
                auto block_handle = detail::allocate_device_memory(_devices.graphics.handle, block_size, memory_type, _vulkan_allocator.vulkan_callbacks());

                _memory_blocks.emplace_back(DeviceMemoryBlock{
                    .device_memory_handle = block_handle,
                    .device_memory_total = block_size,
                    .device_memory_usage = 0,
                });

                _current_memory_blocks.emplace(
                    memory_type,
                    block_index
                );
                _block_info.emplace(block_handle, block_index);
            }

            DeviceMemoryBlock* memory_block = &_memory_blocks[_current_memory_blocks.at(memory_type)];

            auto memory_block_offset = detail::align_block_offset(memory_block->device_memory_usage, static_cast<uint32_t>(alignment));
            if (memory_block_offset + size >= memory_block->device_memory_total)
            {
                uint32_t block_index = static_cast<uint32_t>(_memory_blocks.size());
                auto block_handle = detail::allocate_device_memory(_devices.graphics.handle, block_size, memory_type, _vulkan_allocator.vulkan_callbacks());

                _memory_blocks.emplace_back(DeviceMemoryBlock{
                    .device_memory_handle = detail::allocate_device_memory(_devices.graphics.handle, block_size, memory_type, _vulkan_allocator.vulkan_callbacks()),
                    .device_memory_total = block_size,
                    .device_memory_usage = 0,
                });

                _current_memory_blocks[memory_type] = block_index;
                _block_info.emplace(block_handle, block_index);
            }

            memory_block = &_memory_blocks[_current_memory_blocks.at(memory_type)];
            memory_block_offset = detail::align_block_offset(memory_block->device_memory_usage, static_cast<uint32_t>(alignment));
            IS_ASSERT(memory_block_offset + size <= memory_block->device_memory_total, "Memory block for type {} is insufficient!", memory_type);

            memory_block->device_memory_usage = static_cast<uint32_t>(memory_block_offset + size);

            memory_info.memory_handle = memory_block->device_memory_handle;
            memory_info.memory_offset = static_cast<uint32_t>(memory_block_offset);
            memory_info.memory_size = static_cast<uint32_t>(size);
        }
    }

} // namespace render::vulkan
