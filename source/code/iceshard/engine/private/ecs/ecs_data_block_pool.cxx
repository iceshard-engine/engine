/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/ecs/ecs_data_block_pool.hxx>
#include <ice/assert.hxx>

namespace ice::ecs::detail
{

    class DefaultDataBlockPool final : public ice::ecs::detail::DataBlockPool
    {
    public:
        DefaultDataBlockPool(ice::Allocator& alloc) noexcept;
        ~DefaultDataBlockPool() noexcept override;

        auto provided_block_size() const noexcept -> ice::usize override;
        auto request_block() noexcept -> ice::ecs::detail::DataBlock* override;
        void release_block(ice::ecs::detail::DataBlock* block) noexcept override;

    private:
        ice::Allocator& _allocator;
        ice::ecs::detail::DataBlock* _free_block_list;
    };

    DefaultDataBlockPool::DefaultDataBlockPool(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _free_block_list{ nullptr }
    {
    }

    DefaultDataBlockPool::~DefaultDataBlockPool() noexcept
    {
        ice::ecs::detail::DataBlock* block = _free_block_list;
        while (block != nullptr)
        {
            auto* block_to_release = block;
            block = block->next;

            _allocator.deallocate(
                Memory{
                    .location = block_to_release,
                    .size = Constant_DefaultBlockSize,
                    .alignment = ice::align_of<DataBlock>
                }
            );
        }
    }

    auto DefaultDataBlockPool::provided_block_size() const noexcept -> ice::usize
    {
        return { Constant_DefaultBlockSize.value - ice::size_of<DataBlock>.value };
    }

    auto DefaultDataBlockPool::request_block() noexcept -> ice::ecs::detail::DataBlock*
    {
        DataBlock* free_block = _free_block_list;
        if (free_block != nullptr)
        {
            _free_block_list = free_block->next;
        }

        if (free_block == nullptr)
        {
            void* block_data = _allocator.allocate({ Constant_DefaultBlockSize, ice::align_of<DataBlock> }).memory;
            free_block = reinterpret_cast<DataBlock*>(block_data);
            free_block->block_data_size = provided_block_size();
            free_block->block_data = free_block + 1;
        }

        free_block->block_entity_count_max = 0;
        free_block->block_entity_count = 0;
        free_block->next = nullptr;
        return free_block;
    }

    void DefaultDataBlockPool::release_block(ice::ecs::detail::DataBlock* block) noexcept
    {
        ICE_ASSERT(block->next == nullptr, "Only tail blocks can be released!");

        block->next = _free_block_list;
        _free_block_list = block;
    }

    auto create_default_block_pool(ice::Allocator& alloc) noexcept -> ice::UniquePtr<ice::ecs::detail::DataBlockPool>
    {
        return ice::make_unique<ice::ecs::detail::DefaultDataBlockPool>(alloc, alloc);
    }

} // ice::ecs
