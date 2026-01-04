/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/mem_allocator_snake.hxx>
#include <ice/assert_core.hxx>
#include <bit>
#include <span>

namespace ice
{
    struct SnakeAllocator::ChainBlock
    {
        struct alignas(16) Entry
        {
            ice::u8 free;
            ice::u8 block;
            ice::u8 _unused[2];
            ice::u32 size;
            ice::u8 _unused2[8];

            static auto get_at(Entry* entries, ice::usize bucket_size, ice::u32 idx) noexcept -> Entry*
            {
                return reinterpret_cast<Entry*>(ice::ptr_add(entries, bucket_size * idx));
            }

            static auto count_entries(ice::usize block_size, ice::usize bucket_size) noexcept -> ice::u32
            {
                return ice::u32(block_size.value / bucket_size.value);
            }
        };

        Entry* entries;
    };

    struct SnakeAllocator::Chain
    {
        using Entry = ChainBlock::Entry;

        Chain(ice::usize base_block_size, ice::u32 capacity, ice::usize base_size) noexcept
            : _capacity{ capacity }
            , _count{ capacity }
            , block_size{ base_block_size + ice::size_of<Entry> * Entry::count_entries(base_block_size, base_size) }
            , block_bucket_size{ base_size + ice::size_of<Entry> }
            , block_bucket_count{ Entry::count_entries(block_size, block_bucket_size) }
            , total_bucket_count{ _count * block_bucket_count }
        {
            ICE_ASSERT_CORE(_capacity <= 256);
            ICE_ASSERT_CORE(block_bucket_size < 4_KiB);
        }

        void prepare_block(ice::Allocator& alloc, ChainBlock& block, ice::u8 block_index) const noexcept
        {
            // Allocate memory
            block.entries = reinterpret_cast<ChainBlock::Entry*>(
                alloc.allocate({block_size, ice::ualign::b_16}).memory
            );

            // Setup the entry list
            for (ice::u32 idx = 0; idx < block_bucket_count; ++idx)
            {
                Entry* const entry = Entry::get_at(block.entries, block_bucket_size, idx);
                entry->free = 1;
                entry->block = block_index;
            }
        }

        auto allocate(ice::AllocRequest request) noexcept -> ice::AllocResult
        {
            ice::AllocResult result{ };
            ice::u32 const bucket_idx = allocated.fetch_add(1, std::memory_order_relaxed);

            ice::u32 const block_idx = bucket_idx % _count;
            ice::u32 const block_bucket_idx = (bucket_idx / _count) % block_bucket_count;

            Entry* const entry = Entry::get_at(blocks[block_idx].entries, block_bucket_size, block_bucket_idx);
            if (entry->free == 1) // Check if the slot is empty
            {
                // NOTE: This should actually be a 'atomic' operation, however we "know" that getting to the same slot should at least take some time
                //  for now this is acceptable. Later we might need to use atomics to handle this case properly.
                entry->free = 0;
                entry->size = ice::u32(request.size.value);

                result.size = request.size;
                result.alignment = request.alignment;
                result.memory = entry + 1;
            }
            else
            {
                // If the slot was full we just return empty and go to the fallback allocator.
                released.fetch_add(1, std::memory_order_relaxed);
            }
            return result;
        }

        bool try_deallocate(void* pointer) noexcept
        {
            // Can't release a 'free' pointer (simple early exit cases)
            Entry* const bucket = reinterpret_cast<Entry*>(ice::ptr_sub(pointer, 16_B));
            if (bucket->free == 1 || bucket->block >= _capacity) [[unlikely]]
            {
                return false;
            }

            ice::isize const distance = ice::ptr_offset(blocks[bucket->block].entries, pointer);
            if (distance >= 0_B && distance <= block_size) // Double check we are in the epected memory arena
            {
                bucket->size = 0;
                bucket->free = 1;

                released.fetch_add(1, std::memory_order_relaxed);
                return true;
            }
            return false;
        }

        ice::u32 const _capacity;
        ice::u32 _count = 0;

        ice::usize const block_size;
        ice::usize const block_bucket_size;
        ice::u32 const block_bucket_count;
        ice::u32 const total_bucket_count;

        // NOTE: We run into false sharing here but for now it's not an issue.
        std::atomic_uint32_t allocated = 0;
        std::atomic_uint32_t released = 0;

        ChainBlock* blocks = nullptr;
        Chain* next = nullptr;
    };

    SnakeAllocator::SnakeAllocator(
        ice::Allocator& backing_allocator,
        ice::SnakeAllocatorParams const& params,
        std::source_location src_location
    ) noexcept
        : Allocator{ src_location, backing_allocator }
        , _backing_allocator{ backing_allocator }
        , _chains{ nullptr }
        , _blocks{ nullptr }
    {
        initialize(params);
    }

    SnakeAllocator::SnakeAllocator(
        ice::Allocator& backing_allocator,
        std::string_view name,
        ice::SnakeAllocatorParams const& params,
        std::source_location src_location
    ) noexcept
        : Allocator{ src_location, backing_allocator, name }
        , _backing_allocator{ backing_allocator }
        , _chains{ nullptr }
        , _blocks{ nullptr }
    {
        initialize(params);
    }

    SnakeAllocator::~SnakeAllocator() noexcept
    {
        while(_chains != nullptr)
        {
            Chain* const chain = ice::exchange(_chains, _chains->next);
            ICE_ASSERT_CORE(chain->allocated == chain->released);

            // Release block memory
            for (ice::u32 idx = 0; idx < chain->_count; ++idx)
            {
                _backing_allocator.deallocate(chain->blocks[idx].entries);
            }

            // Destroy the chain object
            _backing_allocator.destroy(chain);
        }

        _backing_allocator.deallocate(_blocks);
    }

    auto SnakeAllocator::do_allocate(ice::AllocRequest request) noexcept -> ice::AllocResult
    {
        ice::AllocResult result{};

        // We only handle 8byte aligned data, everything else goes to the backing allocator
        if (request.alignment <= ice::ualign::b_16 && request.size <= 4_KiB)
        {
            Chain* chain = _chains;
            while(chain != nullptr && chain->block_bucket_size < request.size)
            {
                chain = chain->next;
            }

            // Try to allocate into one of the chains
            if (chain != nullptr)
            {
                result = chain->allocate(request);
            }
        }

        if (result.memory == nullptr) [[unlikely]]
        {
            // Use backing allocator as fallback
            result = _backing_allocator.allocate(request);
        }

        return result;
    }

    void SnakeAllocator::do_deallocate(void* pointer) noexcept
    {
        Chain* chain = _chains;
        while(chain != nullptr && chain->try_deallocate(pointer) == false)
        {
            chain = chain->next;
        }

        // Failed to deallocate, fallback to backing alloc
        if (chain == nullptr)
        {
            _backing_allocator.deallocate(pointer);
        }
    }

    void SnakeAllocator::initialize(ice::SnakeAllocatorParams const& params) noexcept
    {
        ICE_ASSERT_CORE(params.block_sizes.size() == params.bucket_sizes.size());

        _blocks = _backing_allocator.allocate<ChainBlock>(params.bucket_sizes.size() * params.chain_capacity);
        ice::memset(_blocks, 0, sizeof(ChainBlock) * params.bucket_sizes.size() * params.chain_capacity);

        ice::u32 block_offset = 0;
        for (ice::u32 chain_idx = 0; chain_idx < params.bucket_sizes.size(); ++chain_idx)
        {
            // Size is not a power of 2
            ICE_ASSERT_CORE(std::popcount(params.bucket_sizes[chain_idx].value) == 1);
            ICE_ASSERT_CORE(params.chain_capacity <= 256);

            Chain* const chain = _backing_allocator.create<Chain>(
                params.block_sizes[chain_idx],
                params.chain_capacity,
                params.bucket_sizes[chain_idx]
            );
            chain->next = ice::exchange(_chains, chain);
            chain->blocks = _blocks + block_offset;

            // Prepare blocks
            for (ice::u8 block_idx = 0; block_idx < chain->_count; ++block_idx)
            {
                chain->prepare_block(_backing_allocator, chain->blocks[block_idx], block_idx);
            }

            // Move block array offset
            block_offset += params.chain_capacity;
        }
    }

} // namespace ice
