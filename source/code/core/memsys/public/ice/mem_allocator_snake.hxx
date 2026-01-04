/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <span>

namespace ice
{

    struct SnakeAllocatorParams
    {
        static constexpr ice::usize Constant_DefaultBlockSizes[]{ 32_KiB };
        static constexpr ice::usize Constant_DefaultBucketSizes[]{ 256_KiB };

        ice::u32 chain_capacity = 4;

        std::span<ice::usize const> block_sizes = Constant_DefaultBlockSizes;

        std::span<ice::usize const> bucket_sizes = Constant_DefaultBucketSizes;
    };

    struct SnakeAllocator : public ice::Allocator
    {
        SnakeAllocator(
            ice::Allocator& backing_allocator,
            ice::SnakeAllocatorParams const& params = { },
            std::source_location = std::source_location::current()
        ) noexcept;

        SnakeAllocator(
            ice::Allocator& backing_allocator,
            std::string_view name,
            ice::SnakeAllocatorParams const& params = { },
            std::source_location = std::source_location::current()
        ) noexcept;

        ~SnakeAllocator() noexcept;

    protected:
        auto do_allocate(ice::AllocRequest request) noexcept -> ice::AllocResult override;
        void do_deallocate(void* pointer) noexcept override;

        void initialize(ice::SnakeAllocatorParams const& params) noexcept;

    private:
        struct Chain;
        struct ChainBlock;

        ice::Allocator& _backing_allocator;

        Chain* _chains;
        ChainBlock* _blocks;
    };

} // namespace ice
