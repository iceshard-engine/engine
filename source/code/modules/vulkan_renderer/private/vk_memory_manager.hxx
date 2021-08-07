#pragma once
#include <ice/string.hxx>
#include <ice/pod/hash.hxx>
#include <ice/memory.hxx>
#include <ice/memory/scratch_allocator.hxx>
#include <ice/memory/forward_allocator.hxx>
#include "vk_include.hxx"

namespace ice::render::vk
{

    enum class AllocationType : ice::u32
    {
        RenderTarget = 0x0,
        ImageSmall,
        ImageLarge,
        Buffer,
        TransferBuffer,

        Implicit = 0xffff'ffff,
    };

    struct AllocationBlockInfo
    {
        //ice::render::vk::AllocationType type;
        ice::u32 block_size;
        ice::u32 allocation_min;
        ice::u32 allocation_max;
        ice::u32 allocation_stride;
        ice::String name;
    };

    constexpr AllocationBlockInfo defined_allocation_blocks[]{
        AllocationBlockInfo{
            .block_size = 0,
            .allocation_min = 0,
            .allocation_max = 4096 * 4096 * 4,
            .allocation_stride = 256,
            .name = "Render Target Memory"
        },
        AllocationBlockInfo{
            .block_size = 1024 * 1024 * 4,
            .allocation_min = 16 * 16 * 4,
            .allocation_max = 128 * 128 * 4,
            .allocation_stride = 16 * 16 * 4,
            .name = "Small Image Memory"
        },
        AllocationBlockInfo{
            .block_size = 2048 * 2048 * 4 * 2,
            .allocation_min = 128 * 128 * 4 + 1,
            .allocation_max = 2048 * 2048 * 4,
            .allocation_stride = 16 * 16 * 4,
            .name = "Large Image Memory"
        },
        AllocationBlockInfo{
            .block_size = 1024 * 1024 * 16,
            .allocation_min = 1024,
            .allocation_max = 1024 * 1024 * 4,
            .allocation_stride = 256,
            .name = "Buffer Memory"
        },
        AllocationBlockInfo{
            .block_size = 0,
            .allocation_min = 16 * 16 * 4,
            .allocation_max = 4096 * 4096 * 4,
            .allocation_stride = 256,
            .name = "Transfer Buffer Memory"
        },
    };

    struct AllocationEntry
    {
        ice::u32 size = 0;
        ice::u32 offset = 0;
        AllocationEntry* next = nullptr;
    };

    struct AllocationBlock
    {;
        ice::i16 memory_type_index = -1;
        ice::u16 block_identifier = 0;
        AllocationBlockInfo const* info = nullptr;
        AllocationEntry* free = nullptr;
        AllocationEntry* used = nullptr;

        VkDeviceMemory memory_handle;
    };

    struct AllocationInfo
    {
        ice::u32 size;
        ice::u32 offset;
        VkDeviceMemory memory_handle;
    };

    enum class AllocationHandle : ice::uptr
    {
        Invalid = 0x0
    };

    class VulkanMemoryManager final
    {
    public:
        VulkanMemoryManager(
            ice::Allocator& alloc,
            VkDevice device,
            VkPhysicalDeviceMemoryProperties const& memory_properties
        ) noexcept;
        ~VulkanMemoryManager() noexcept;

        auto allocate(
            VkImage image,
            VkMemoryPropertyFlags flags,
            AllocationType type,
            AllocationInfo& info_out
        ) noexcept -> AllocationHandle;

        auto allocate(
            VkBuffer image,
            VkMemoryPropertyFlags flags,
            AllocationType type,
            AllocationInfo& info_out
        ) noexcept -> AllocationHandle;

        void release(AllocationHandle handle) noexcept;

        void map_memory(
            ice::Span<AllocationHandle const> handles,
            ice::Span<ice::Memory> out_data
        ) noexcept;

        void unmap_memory(
            ice::Span<AllocationHandle const> handles
        ) noexcept;

    protected:
        bool find_block_and_entry(
            AllocationHandle handle,
            AllocationBlock*& block,
            AllocationEntry*& entry
        ) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::memory::ScratchAllocator _map_allocator;
        ice::memory::ForwardAllocator _block_allocator;
        ice::memory::ForwardAllocator _entry_allocator;
        ice::pod::Hash<AllocationBlock*> _blocks;

        VkDevice _vk_device;
        VkPhysicalDeviceMemoryProperties const& _vk_physical_device_memory_properties;
    };

} // namespace ice::render::vk
