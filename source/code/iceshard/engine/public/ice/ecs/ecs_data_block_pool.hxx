/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/container_types.hxx>
#include <ice/ecs/ecs_data_block.hxx>

namespace ice::ecs
{

    static constexpr ice::usize Constant_DefaultBlockSize = 32_KiB;

    class DataBlockPool
    {
    public:
        DataBlockPool(ice::Allocator& alloc) noexcept;
        ~DataBlockPool() noexcept;

        auto provided_block_size() const noexcept -> ice::usize;

        auto request_block() noexcept -> ice::ecs::DataBlock*;
        void release_block(ice::ecs::DataBlock* block) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::ecs::DataBlock* _free_block_list;
    };

} // namespace ice::ecs
