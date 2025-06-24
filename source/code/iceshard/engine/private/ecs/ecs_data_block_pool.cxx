/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/ecs/ecs_data_block_pool.hxx>
#include <ice/ecs/ecs_archetype_detail.hxx>
#include <ice/assert.hxx>

namespace ice::ecs::detail
{

    class DefaultDataBlockPool final : public ice::ecs::detail::DataBlockPool
    {
    public:
        DefaultDataBlockPool(ice::Allocator& alloc) noexcept;
        ~DefaultDataBlockPool() noexcept override;

        auto provided_block_size() const noexcept -> ice::usize override;
        auto request_block(ice::ecs::detail::ArchetypeInstanceInfo const& info) noexcept -> ice::ecs::detail::DataBlock* override;
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

    auto DefaultDataBlockPool::request_block(ice::ecs::detail::ArchetypeInstanceInfo const& info) noexcept -> ice::ecs::detail::DataBlock*
    {
        DataBlock* free_block = _free_block_list;
        if (free_block != nullptr)
        {
            _free_block_list = free_block->next;
        }

        if (free_block == nullptr)
        {
            ice::usize filter_data_size = info.data_block_filter.data_size;

            void* block_data = _allocator.allocate({ Constant_DefaultBlockSize, ice::align_of<DataBlock> }).memory;
            free_block = reinterpret_cast<DataBlock*>(block_data);
            free_block->block_data_size = ice::usize::subtract(provided_block_size(), filter_data_size);
            free_block->block_filter_data = filter_data_size > 0_B ? (free_block + 1) : nullptr;
            free_block->block_data = ice::ptr_add(free_block + 1, filter_data_size);// free_block + 1;
        }

        free_block->block_entity_count_max = ice::ecs::detail::calculate_entity_count_for_space(info, provided_block_size());
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
