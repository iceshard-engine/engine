#include <ice/ecs/ecs_data_block_pool.hxx>
#include <ice/assert.hxx>

namespace ice::ecs
{

    DataBlockPool::DataBlockPool(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _free_block_list{ nullptr }
    {
    }

    DataBlockPool::~DataBlockPool() noexcept
    {
        ice::ecs::DataBlock* block = _free_block_list;
        while (block != nullptr)
        {
            auto* block_to_release = block;
            block = block->next;

            _allocator.deallocate(block_to_release);
        }
    }

    auto DataBlockPool::provided_block_size() const noexcept -> ice::u32
    {
        return Constant_DefaultBlockSize - sizeof(DataBlock);
    }

    auto DataBlockPool::request_block() noexcept -> ice::ecs::DataBlock*
    {
        DataBlock* free_block = _free_block_list;
        if (free_block != nullptr)
        {
            _free_block_list = free_block->next;
        }

        if (free_block == nullptr)
        {
            void* block_data = _allocator.allocate(Constant_DefaultBlockSize, alignof(DataBlock));
            free_block = reinterpret_cast<DataBlock*>(block_data);
            free_block->block_data_size = Constant_DefaultBlockSize - sizeof(DataBlock);
            free_block->block_data = free_block + 1;
        }

        free_block->block_entity_count_max = 0;
        free_block->block_entity_count = 0;
        free_block->next = nullptr;
        return free_block;
    }

    void DataBlockPool::release_block(ice::ecs::DataBlock* block) noexcept
    {
        ICE_ASSERT(block->next == nullptr, "Only tail blocks can be released!");

        block->next = _free_block_list;
        _free_block_list = block;
    }

} // ice::ecs
