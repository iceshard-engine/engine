#pragma once
#include <ice/allocator.hxx>
#include <ice/pod/collections.hxx>
#include <ice/ecs/ecs_data_block.hxx>

namespace ice::ecs
{

    class DataBlockPool
    {
    public:
        DataBlockPool(ice::Allocator& alloc) noexcept;
        ~DataBlockPool() noexcept;

        auto request_block() noexcept -> ice::ecs::DataBlock*;
        void release_block(ice::ecs::DataBlock* block) noexcept;

    private:
        ice::Allocator& _allocator;

        std::atomic<ice::ecs::DataBlock*> _free_block_stack;
    };

} // namespace ice::ecs
