#include <ice/mem_allocator_snake.hxx>
#include <ice/assert_core.hxx>
#include <bit>
#include <span>

namespace ice
{

    namespace detail
    {

        auto calc_required_tracking_buckets(ice::ucount bucket_count, ice::usize bucket_size) noexcept -> ice::ucount
        {
            // TODO: Fix
            // ice::ucount const required_tracking_bytes = bucket_count / 8; // 8 buckets for each byte
            ice::ucount const required_tracking_buckets = bucket_size > 64_B ? 2 : 32;
            // ice::ucount const required_tracking_buckets = ice::ucount(required_tracking_bytes / bucket_size.value)
            //     + ice::ucount((bucket_size.value % required_tracking_bytes) != 0); // how many buckets less.
            return required_tracking_buckets;
        }

    } // namespace detail

    struct SnakeAllocator::ChainBlock
    {
        char* tracking; // TODO: Make atomic?
        char* memory;
    };

    struct SnakeAllocator::Chain
    {
        Chain(ice::usize block_size, ice::ucount capacity, ice::usize size) noexcept
            : _capacity{ capacity }
            , _count{ capacity }
            , block_size{ block_size }
            , block_bucket_size{ size }
            , block_bucket_count{ ice::ucount(block_size.value / size.value) }
            , block_tracking_bucket_count{ detail::calc_required_tracking_buckets(block_bucket_count, block_bucket_size) }
            , block_data_bucket_count{ block_bucket_count - block_tracking_bucket_count }
        {
        }

        void prepare_block(ice::Allocator& alloc, ChainBlock& block) const noexcept
        {
            // Allocate memory
            block.tracking = (char*) alloc.allocate(block_size).memory;

            // Move pointer by bucket aligned sized offset
            block.memory = block.tracking + (block_tracking_bucket_count * block_bucket_size.value);

            // Clear the tracking data part
            ice::memset(block.tracking, 0, block_tracking_bucket_count * block_bucket_size.value);
        }

        auto allocate(ice::AllocRequest request) noexcept -> ice::AllocResult
        {
            ice::AllocResult result{ };
            do
            {
                ice::u32 const bucket_idx = allocated.fetch_add(1, std::memory_order_relaxed);
                ice::u32 const block_idx = bucket_idx % _count;
                ice::u32 const bucket_offset = (bucket_idx / _count) % block_data_bucket_count;
                ice::usize const memory_offset = block_bucket_size * bucket_offset;

                // Check if the bucket is empty (get the byte then the bit)
                ice::u32 const tracking_byte_idx = bucket_offset / 8;
                ice::u32 const tracking_bit = 0b1 << (bucket_offset % 8);
                char& tracking_byte = blocks[block_idx].tracking[tracking_byte_idx];

                // Assing the result and the tracking bit
                if ((tracking_byte & tracking_bit) == 0)
                {
                    tracking_byte |= tracking_bit;

                    result.size = request.size;
                    result.alignment = request.alignment;
                    result.memory = ice::ptr_add(blocks[block_idx].memory, memory_offset);
                }
                else
                {
                    // If not allocated this counts as a 'release'
                    released.fetch_add(1, std::memory_order_relaxed);
                }

            } while (result.memory == nullptr);
            return result;
        }

        bool try_deallocate(void* pointer) noexcept
        {
            ice::isize const available_block_size = block_size - (block_bucket_size * block_tracking_bucket_count);

            for (ice::u32 idx = 0; idx < _count; ++idx)
            {
                void const* const begin = blocks[idx].memory;

                ice::isize const distance = ice::ptr_offset(begin, pointer);
                if (distance >= 0_B && distance <= available_block_size)
                {
                    // Check if the bucket is taken (get the byte then the bit)
                    ice::u32 const bucket_index = ice::u32(distance.value / block_bucket_size.value);
                    ice::u32 const tracking_byte_idx = bucket_index / 8;
                    ice::u32 const tracking_bit = 0b1 << (bucket_index % 8);

                    char& tracking_byte = blocks[idx].tracking[tracking_byte_idx];
                    ICE_ASSERT_CORE((tracking_byte & tracking_bit) != 0);

                    // Update the byte
                    tracking_byte = tracking_byte & ~tracking_bit;

                    // Increment released counter and return, we finished
                    released.fetch_add(1, std::memory_order_relaxed);
                    return true;
                }
            }
            return false;
        }

        ice::ucount const _capacity;
        ice::ucount _count = 0;

        ice::usize const block_size;
        ice::usize const block_bucket_size;
        ice::ucount const block_bucket_count;
        ice::ucount const block_tracking_bucket_count;
        ice::ucount const block_data_bucket_count;


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
                _backing_allocator.deallocate(chain->blocks[idx].tracking);
            }

            // Destroy the chain object
            _backing_allocator.destroy(chain);
        }

        _backing_allocator.deallocate(_blocks);
    }

    auto SnakeAllocator::do_allocate(ice::AllocRequest request) noexcept -> ice::AllocResult
    {
        Chain* chain = _chains;
        while(chain != nullptr && chain->block_bucket_size < request.size)
        {
            chain = chain->next;
        }

        // Try to allocate into one of the chains
        ice::AllocResult result{};
        if (chain != nullptr)
        {
            result = chain->allocate(request);
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

        ice::ucount block_offset = 0;
        for (ice::u32 chain_idx = 0; chain_idx < params.bucket_sizes.size(); ++chain_idx)
        {
            // Size is not a power of 2
            ICE_ASSERT_CORE(std::popcount(params.bucket_sizes[chain_idx].value) == 1);

            Chain* const chain = _backing_allocator.create<Chain>(
                params.block_sizes[chain_idx],
                params.chain_capacity,
                params.bucket_sizes[chain_idx]
            );
            chain->next = ice::exchange(_chains, chain);
            chain->blocks = _blocks + block_offset;

            // Prepare blocks
            for (ice::u32 block_idx = 0; block_idx < chain->_count; ++block_idx)
            {
                chain->prepare_block(_backing_allocator, chain->blocks[block_idx]);
            }

            // Move block array offset
            block_offset += params.chain_capacity;
        }
    }

} // namespace ice
