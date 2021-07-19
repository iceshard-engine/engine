#include "vk_memory_manager.hxx"
#include "vk_utility.hxx"
#include <ice/memory/pointer_arithmetic.hxx>
#include <ice/assert.hxx>

namespace ice::render::vk
{

    namespace detail
    {

        void get_memory_requirements_and_index(
            VkDevice vk_device,
            VkImage vk_image,
            VkMemoryPropertyFlags flags,
            VkPhysicalDeviceMemoryProperties const& memory_properties,
            VkMemoryRequirements& requirements_out,
            ice::i32& index_out
        ) noexcept
        {
            vkGetImageMemoryRequirements(vk_device, vk_image, &requirements_out);

            index_out = -1;
            for (ice::i32 i = 0; i < memory_properties.memoryTypeCount; ++i)
            {
                if ((requirements_out.memoryTypeBits & 0x1) == 0x1)
                {
                    // Type is available, does it match user properties?
                    if ((memory_properties.memoryTypes[i].propertyFlags & flags) == flags)
                    {
                        index_out = i;
                        break;
                    }
                }
                requirements_out.memoryTypeBits >>= 1;
            }

            ICE_ASSERT(
                index_out != -1,
                "No valid memory type found!"
            );
        }

        void get_memory_requirements_and_index(
            VkDevice vk_device,
            VkBuffer vk_buffer,
            VkMemoryPropertyFlags flags,
            VkPhysicalDeviceMemoryProperties const& memory_properties,
            VkMemoryRequirements& requirements_out,
            ice::i32& index_out
        ) noexcept
        {
            vkGetBufferMemoryRequirements(vk_device, vk_buffer, &requirements_out);

            index_out = -1;
            for (ice::i32 i = 0; i < memory_properties.memoryTypeCount; ++i)
            {
                if ((requirements_out.memoryTypeBits & 0x1) == 0x1)
                {
                    // Type is available, does it match user properties?
                    if ((memory_properties.memoryTypes[i].propertyFlags & flags) == flags)
                    {
                        index_out = i;
                        break;
                    }
                }
                requirements_out.memoryTypeBits >>= 1;
            }

            ICE_ASSERT(
                index_out != -1,
                "No valid memory type found!"
            );
        }

        static ice::u16 global_block_identifier = 1;

        void ensure_free_block_exists(
            VkDevice device,
            ice::u32 required_size,
            ice::i32 memory_type_index,
            ice::render::vk::AllocationBlockInfo const* info,
            ice::memory::ForwardAllocator& block_alloc,
            ice::memory::ForwardAllocator& entry_alloc,
            ice::pod::Hash<AllocationBlock*>& blocks
        ) noexcept
        {
            bool found_free_block = false;
            auto* it = ice::pod::multi_hash::find_first(blocks, memory_type_index);
            while (it != nullptr && found_free_block == false)
            {
                if (it->value->info == info)
                {
                    AllocationEntry* entry = it->value->free;
                    while (entry != nullptr && found_free_block == false)
                    {
                        found_free_block = entry->size >= required_size;
                        entry = entry->next;
                    }
                }

                it = ice::pod::multi_hash::find_next(blocks, it);
            }

            if (found_free_block == false)
            {
                VkMemoryAllocateInfo alloc_info = {};
                alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                alloc_info.pNext = nullptr;
                alloc_info.allocationSize = info->block_size == 0 ? required_size : info->block_size;
                alloc_info.memoryTypeIndex = memory_type_index;

                VkDeviceMemory memory_handle;
                VkResult api_result = vkAllocateMemory(device, &alloc_info, nullptr, &memory_handle);
                ICE_ASSERT(
                    api_result == VkResult::VK_SUCCESS,
                    "Couldn't allocate memory for image object!"
                );

                AllocationEntry* entry = entry_alloc.make<AllocationEntry>();
                entry->next = nullptr;
                entry->offset = 0;
                entry->size = alloc_info.allocationSize;

                AllocationBlock* block = block_alloc.make<AllocationBlock>();
                block->memory_type_index = static_cast<ice::i16>(memory_type_index);
                block->block_identifier = global_block_identifier++;
                block->info = info;
                block->free = entry;
                block->used = nullptr;
                block->memory_handle = memory_handle;

                ice::pod::multi_hash::insert(
                    blocks,
                    memory_type_index,
                    block
                );
            }
        }

        void select_block_and_entry(
            ice::u32 required_size,
            ice::i32 memory_type_index,
            ice::render::vk::AllocationBlockInfo const* info,
            ice::memory::ForwardAllocator& entry_alloc,
            ice::pod::Hash<AllocationBlock*>& blocks,
            ice::render::vk::AllocationBlock*& selected_block_out,
            ice::render::vk::AllocationEntry*& selected_entry_out
        ) noexcept
        {
            ice::u32 alloc_size = required_size;
            ice::u32 stride_modulo = required_size % info->allocation_stride;
            if (stride_modulo > 0)
            {
                alloc_size += info->allocation_stride - stride_modulo;
            }

            auto* it = ice::pod::multi_hash::find_first(blocks, memory_type_index);
            while (it != nullptr && selected_block_out == nullptr)
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
                            AllocationEntry* free_entry = entry_alloc.make<AllocationEntry>();
                            free_entry->next = nullptr;
                            free_entry->offset = candidate_entry->offset + alloc_size;
                            free_entry->size = remaining_size;

                            if (parent_entry != nullptr)
                            {
                                free_entry->next = parent_entry->next;
                                parent_entry->next = free_entry;
                            }
                            else
                            {
                                candidate_block->free = free_entry;
                            }
                            candidate_entry->size = alloc_size;
                        }

                        selected_block_out = candidate_block;
                        selected_entry_out = candidate_entry;
                    }
                }
                it = ice::pod::multi_hash::find_next(blocks, it);
            }

            ICE_ASSERT(
                selected_block_out != nullptr,
                "No memory block was selected for allocation!"
            );
            ICE_ASSERT(
                selected_entry_out != nullptr,
                "No memory entry was selected for allocation!"
            );
        }

    } // namespace detail

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
        , _map_allocator{ _allocator, 1024 * 1024 * 2 }
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
        ice::i32 memory_type_index;
        VkMemoryRequirements memory_requirements;
        detail::get_memory_requirements_and_index(
            _vk_device,
            image,
            flags,
            _vk_physical_device_memory_properties,
            memory_requirements,
            memory_type_index
        );

        if (type == AllocationType::Implicit)
        {
            type = AllocationType::ImageSmall;
        }

        ice::u32 const size = static_cast<ice::u32>(memory_requirements.size);
        ice::render::vk::AllocationBlockInfo const* info = &defined_allocation_blocks[static_cast<ice::u32>(type)];
        if (info->allocation_max < size)
        {
            info = &defined_allocation_blocks[ice::u32(AllocationType::ImageLarge)];
            ICE_ASSERT(
                size <= info->allocation_max,
                "Requested image memory size too big!"
            );
        };

        detail::ensure_free_block_exists(
            _vk_device,
            size,
            memory_type_index,
            info,
            _block_allocator,
            _entry_allocator,
            _blocks
        );

        AllocationBlock* selected_block = nullptr;
        AllocationEntry* selected_entry = nullptr;
        detail::select_block_and_entry(
            size,
            memory_type_index,
            info,
            _entry_allocator,
            _blocks,
            selected_block,
            selected_entry
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

    auto VulkanMemoryManager::allocate(
        VkBuffer buffer,
        VkMemoryPropertyFlags flags,
        AllocationType type,
        AllocationInfo& info_out
    ) noexcept -> AllocationHandle
    {
        ice::i32 memory_type_index;
        VkMemoryRequirements memory_requirements;
        detail::get_memory_requirements_and_index(
            _vk_device,
            buffer,
            flags,
            _vk_physical_device_memory_properties,
            memory_requirements,
            memory_type_index
        );

        if (type == AllocationType::Implicit)
        {
            type = AllocationType::Buffer;
        }

        ice::u32 const size = static_cast<ice::u32>(memory_requirements.size);
        ice::render::vk::AllocationBlockInfo const* info = &defined_allocation_blocks[static_cast<ice::u32>(type)];
        ICE_ASSERT(
            size <= info->allocation_max,
            "Requested buffer size too big!"
        );

        detail::ensure_free_block_exists(
            _vk_device,
            size,
            memory_type_index,
            info,
            _block_allocator,
            _entry_allocator,
            _blocks
        );

        AllocationBlock* selected_block = nullptr;
        AllocationEntry* selected_entry = nullptr;
        detail::select_block_and_entry(
            size,
            memory_type_index,
            info,
            _entry_allocator,
            _blocks,
            selected_block,
            selected_entry
        );

        // Push the new entry at the top of the list
        selected_entry->next = selected_block->used;
        selected_block->used = selected_entry;

        // Bind the memory to the image handle
        VkResult api_result = vkBindBufferMemory(
            _vk_device,
            buffer,
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
        while (it != nullptr && selected_block == nullptr)
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

    void VulkanMemoryManager::map_memory(
        ice::Span<AllocationHandle const> handles,
        ice::Span<ice::Memory> out_data
    ) noexcept
    {
        ICE_ASSERT(
            ice::size(handles) == ice::size(out_data),
            "Missmatched handle and out_data span size!"
        );

        struct DeviceMappingEntry
        {
            VkDeviceMemory device_memory;
            VkDeviceSize offset;
            VkDeviceSize size;
        };

        // [issue #49]: This part of the code is generally run on a graphics thread. Because of this we need to have a dedicated memory pool / allocator. So it won't interfer with
        //  other allocations done in the mean time on any other thrads.
        //  We should probably slowly invest some time into thread safe allocators.
        ice::pod::Array<DeviceMappingEntry> vk_mapping_entries{ _map_allocator };
        ice::pod::array::reserve(vk_mapping_entries, ice::size(handles));

        auto push_back_unique_and_update = [](auto& array_, VkDeviceMemory vk_memory, VkDeviceSize offset, VkDeviceSize size) noexcept
        {
            ice::u32 const array_size = ice::pod::array::size(array_);
            for (ice::u32 idx = 0; idx < array_size; ++idx)
            {
                if (array_[idx].device_memory == vk_memory)
                {
                    DeviceMappingEntry& entry = array_[idx];
                    if (entry.offset > offset)
                    {
                        ice::u32 const total_size = (entry.offset - offset) + entry.size;
                        entry.size = total_size;
                        entry.offset = offset;
                    }
                    else
                    {
                        ice::u32 const size_diff = (offset - entry.offset) + size;
                        if (size_diff > entry.size)
                        {
                            //VK_LOG(
                            //    ice::LogSeverity::Warning,
                            //    "Unexpected size difference! [ new:{}, old:{} ]",
                            //    size_diff, entry.size
                            //);
                            entry.size = size_diff;
                        }
                    }
                    return;
                }
            }

            ice::pod::array::push_back(array_,
                DeviceMappingEntry
                {
                    .device_memory = vk_memory,
                    .offset = offset,
                    .size = size
                }
            );
        };

        for (AllocationHandle handle : handles)
        {
            AllocationBlock* block = nullptr;
            AllocationEntry* entry = nullptr;
            find_block_and_entry(handle, block, entry);

            push_back_unique_and_update(
                vk_mapping_entries,
                block->memory_handle,
                entry->offset,
                entry->size
            );
        }

        for (DeviceMappingEntry const& mapping_entry : vk_mapping_entries)
        {
            void* block_ptr = nullptr;
            VkResult result = vkMapMemory(
                _vk_device,
                mapping_entry.device_memory,
                mapping_entry.offset,
                mapping_entry.size,
                0, &block_ptr
            );
            ICE_ASSERT(
                result == VkResult::VK_SUCCESS,
                "Mapping memory block failed!"
            );

            ice::u32 const handle_count = ice::size(handles);
            for (ice::u32 idx = 0; idx < handle_count; ++idx)
            {
                AllocationBlock* block = nullptr;
                AllocationEntry* entry = nullptr;
                find_block_and_entry(handles[idx], block, entry);

                if (block->memory_handle == mapping_entry.device_memory)
                {
                    out_data[idx].location = ice::memory::ptr_add(block_ptr, entry->offset - mapping_entry.offset);
                    out_data[idx].size = entry->size;
                    out_data[idx].alignment = 0;
                }
            }
        }
    }

    void VulkanMemoryManager::unmap_memory(
        ice::Span<AllocationHandle const> handles
    ) noexcept
    {
        // [issue #49]: This part of the code is generally run on a graphics thread. Because of this we need to have a dedicated memory pool / allocator. So it won't interfer with
        //  other allocations done in the mean time on any other thrads.
        //  We should probably slowly invest some time into thread safe allocators.
        ice::pod::Array<VkDeviceMemory> vk_unmapped_entries{ _map_allocator };
        ice::pod::array::reserve(vk_unmapped_entries, ice::size(handles));

        auto unmap_unique = [&](auto& array_, VkDeviceMemory vk_memory) noexcept
        {
            bool found = false;
            ice::u32 const array_size = ice::pod::array::size(array_);
            for (ice::u32 idx = 0; idx < array_size && !found; ++idx)
            {
                if (array_[idx] == vk_memory)
                {
                    found = true;
                }
            }

            if (found == false)
            {
                vkUnmapMemory(
                    _vk_device,
                    vk_memory
                );

                ice::pod::array::push_back(
                    array_,
                    vk_memory
                );
            }
        };

        for (AllocationHandle handle : handles)
        {
            AllocationBlock* block = nullptr;
            AllocationEntry* entry = nullptr;
            find_block_and_entry(handle, block, entry);

            unmap_unique(
                vk_unmapped_entries,
                block->memory_handle
            );
        }
    }

    bool VulkanMemoryManager::find_block_and_entry(
        AllocationHandle handle,
        AllocationBlock*& block_out,
        AllocationEntry*& entry_out
    ) noexcept
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
        while (it != nullptr && selected_block == nullptr)
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
            "Provided memory handle is invalid!"
        );

        block_out = selected_block;
        entry_out = selected_entry;
        return true;
    }

} // namespace ice::render::vk
