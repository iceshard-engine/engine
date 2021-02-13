#include "vk_memory_manager.hxx"
#include <ice/assert.hxx>

namespace ice::render::vk
{

    union AllocationHandleHelper
    {
        struct
        {
            ice::u16 handle_high_h;
            ice::u16 handle_high_l;
            ice::u32 handle_low;
        };
        AllocationHandle handle;
    };

    VulkanMemoryManager::VulkanMemoryManager(
        ice::Allocator& alloc,
        VkDevice device,
        VkPhysicalDeviceMemoryProperties const& memory_properties
    ) noexcept
        : _allocator{ alloc }
        , _block_allocator{ _allocator, sizeof(AllocationBlock) * 8 }
        , _entry_allocator{ _allocator, sizeof(AllocationEntry) * 32 }
        , _blocks{ _allocator }
        , _vk_device{ device }
        , _vk_physical_device_memory_properties{ memory_properties }
    {
    }

    VulkanMemoryManager::~VulkanMemoryManager() noexcept
    {
        for (auto block_entry : _blocks)
        {
            AllocationBlock* const block = block_entry.value;
            ICE_ASSERT(
                block->used == nullptr,
                "Graphics device memory was not properly released!"
            );

            vkFreeMemory(_vk_device, block->memory_handle, nullptr);
        }

    }

    auto VulkanMemoryManager::allocate(
        VkImage image,
        VkMemoryPropertyFlags flags,
        AllocationType type,
        AllocationInfo& info_out
    ) noexcept -> AllocationHandle
    {
        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(_vk_device, image, &memory_requirements);

        ice::i32 memory_type_index = -1;
        for (ice::i32 i = 0; i < _vk_physical_device_memory_properties.memoryTypeCount; ++i)
        {
            if ((memory_requirements.memoryTypeBits & 0x1) == 0x1)
            {
                // Type is available, does it match user properties?
                if ((_vk_physical_device_memory_properties.memoryTypes[i].propertyFlags & flags) == flags)
                {
                    memory_type_index = i;
                    break;
                }
            }
            memory_requirements.memoryTypeBits >>= 1;
        }

        ICE_ASSERT(
            memory_type_index != -1,
            "No valid memory type found!"
        );

        if (type == AllocationType::Implicit)
        {
            type = AllocationType::ImageSmall;
        }

        ice::u32 const size = static_cast<ice::u32>(memory_requirements.size);

        AllocationBlockInfo const* info = &defined_allocation_blocks[ice::u32()];
        if (info->allocation_max < size)
        {
            info = &defined_allocation_blocks[ice::u32(AllocationType::ImageLarge)];
            ICE_ASSERT(
                size <= info->allocation_max,
                "Requested image memory size too big!"
            );
        };

        {
            bool found_free_block = false;
            auto* it = ice::pod::multi_hash::find_first(_blocks, memory_type_index);
            while (it != nullptr && found_free_block == false)
            {
                AllocationEntry* entry = it->value->free;
                while (entry != nullptr && found_free_block == false)
                {
                    found_free_block = entry->size >= size;
                    entry = entry->next;
                }

                it = ice::pod::multi_hash::find_next(_blocks, it);
            }

            if (found_free_block == false)
            {
                VkMemoryAllocateInfo alloc_info = {};
                alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                alloc_info.pNext = nullptr;
                alloc_info.allocationSize = info->block_size == 0 ? size : info->block_size;
                alloc_info.memoryTypeIndex = memory_type_index;

                VkDeviceMemory memory_handle;
                VkResult api_result = vkAllocateMemory(_vk_device, &alloc_info, nullptr, &memory_handle);
                ICE_ASSERT(
                    api_result == VkResult::VK_SUCCESS,
                    "Couldn't allocate memory for image object!"
                );

                AllocationEntry* entry = _entry_allocator.make<AllocationEntry>();
                entry->next = nullptr;
                entry->offset = 0;
                entry->size = alloc_info.allocationSize;

                static ice::u16 global_block_identifier = 1;

                AllocationBlock* block = _block_allocator.make<AllocationBlock>();
                block->memory_type_index = static_cast<ice::i16>(memory_type_index);
                block->block_identifier = global_block_identifier++;
                block->info = info;
                block->free = entry;
                block->used = nullptr;
                block->memory_handle = memory_handle;

                ice::pod::multi_hash::insert(
                    _blocks,
                    memory_type_index,
                    block
                );
            }
        }

        ice::u32 alloc_size = size;
        ice::u32 stride_modulo = size % info->allocation_stride;
        if (stride_modulo > 0)
        {
            alloc_size += info->allocation_stride - stride_modulo;
        }

        AllocationBlock* selected_block = nullptr;
        AllocationEntry* selected_entry = nullptr;

        auto* it = ice::pod::multi_hash::find_first(_blocks, memory_type_index);
        while (it != nullptr || selected_block == nullptr)
        {
            AllocationBlock* candidate_block = it->value;
            if (candidate_block->info == info)
            {
                // Find a suiting entry from the free list
                AllocationEntry* next_entry = candidate_block->free;
                AllocationEntry* candidate_entry = nullptr;
                while (next_entry != nullptr)
                {
                    if (next_entry->size >= alloc_size)
                    {
                        if (candidate_entry == nullptr || next_entry->size <= candidate_entry->size)
                        {
                            candidate_entry = next_entry;
                        }
                    }
                    next_entry = next_entry->next;
                }

                // Remove the entry from the free block
                if (candidate_entry != nullptr)
                {
                    AllocationEntry* parent_entry = candidate_block->free;
                    while (parent_entry != nullptr && parent_entry->next != candidate_entry)
                    {
                        parent_entry = parent_entry->next;
                    }

                    if (parent_entry == nullptr)
                    {
                        candidate_block->free = nullptr;
                    }
                    else
                    {
                        parent_entry->next = candidate_entry->next;
                    }

                    ice::u32 const remaining_size = candidate_entry->size - alloc_size;
                    if (remaining_size > 0 && remaining_size >= info->allocation_min)
                    {
                        AllocationEntry* free_entry = _entry_allocator.make<AllocationEntry>();
                        free_entry->next = parent_entry->next;
                        free_entry->offset = candidate_entry->offset + alloc_size;
                        free_entry->size = remaining_size;

                        parent_entry->next = free_entry;
                        candidate_entry->size = alloc_size;
                    }

                    selected_block = candidate_block;
                    selected_entry = candidate_entry;
                }
            }
            it = ice::pod::multi_hash::find_next(_blocks, it);
        }

        ICE_ASSERT(
            selected_block != nullptr,
            "No memory block was selected for allocation!"
        );
        ICE_ASSERT(
            selected_entry != nullptr,
            "No memory entry was selected for allocation!"
        );

        // Push the new entry at the top of the list
        selected_entry->next = selected_block->used;
        selected_block->used = selected_entry;

        // Bind the memory to the image handle
        VkResult api_result = vkBindImageMemory(
            _vk_device,
            image,
            selected_block->memory_handle,
            selected_entry->offset
        );
        ICE_ASSERT(
            api_result == VkResult::VK_SUCCESS,
            "Couldn't bind memory to image!"
        );

        info_out = AllocationInfo{
            .size = selected_entry->size,
            .offset = selected_entry->offset,
            .memory_handle = selected_block->memory_handle,
        };

        AllocationHandleHelper handle_helper{ };
        handle_helper.handle_high_h = static_cast<ice::u16>(selected_block->memory_type_index);
        handle_helper.handle_high_l = selected_block->block_identifier;
        handle_helper.handle_low = selected_entry->offset;
        return handle_helper.handle;
    }

    void VulkanMemoryManager::release(AllocationHandle handle) noexcept
    {
        AllocationHandleHelper handle_helper{ };
        handle_helper.handle = handle;

        ice::i32 const memory_type_index = static_cast<ice::i16>(handle_helper.handle_high_h);
        ice::u16 const block_identifier = handle_helper.handle_high_l;
        ice::u32 const memory_offset = handle_helper.handle_low;

        AllocationBlock* selected_block = nullptr;
        AllocationEntry* parent_entry = nullptr;
        AllocationEntry* selected_entry = nullptr;

        auto* it = ice::pod::multi_hash::find_first(_blocks, memory_type_index);
        while (it != nullptr || selected_block == nullptr)
        {
            AllocationBlock* candidate_block = it->value;
            if (candidate_block->block_identifier == block_identifier)
            {
                selected_block = candidate_block;
                selected_entry = selected_block->used;
                while (selected_entry->offset != memory_offset)
                {
                    parent_entry = selected_entry;
                    selected_entry = selected_entry->next;
                }
                break;
            }

            it = ice::pod::multi_hash::find_next(_blocks, it);
        }

        ICE_ASSERT(
            selected_entry != nullptr,
            "Selected memory entry is invalid!"
        );

        // Remove from the used list
        if (parent_entry != nullptr)
        {
            parent_entry->next = selected_entry->next;
        }
        else
        {
            selected_block->used = selected_entry->next;
        }

        selected_entry->next = selected_block->free;
        selected_block->free = selected_entry;
    }

} // namespace ice::render::vk
