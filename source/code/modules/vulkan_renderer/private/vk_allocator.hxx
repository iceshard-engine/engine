/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "vk_include.hxx"
#include <ice/mem_allocator.hxx>
#include <ice/mem_allocator_proxy.hxx>
#include <ice/container/hashmap.hxx>

namespace ice::render::vk
{

    //! \brief A special allocator for vulkan allocations.
    class VulkanAllocator final : public ice::Allocator
    {
    public:
        VulkanAllocator(
            ice::Allocator& backing_allocator,
            std::source_location = std::source_location::current()
        ) noexcept;
        ~VulkanAllocator() noexcept override;

        auto vulkan_callbacks() const noexcept -> VkAllocationCallbacks const*;

    public:
        auto do_allocate(ice::AllocRequest req) noexcept -> ice::AllocResult override;
        auto do_reallocate(void* mem, ice::AllocRequest req) noexcept -> ice::AllocResult;
        void do_deallocate(void* mem) noexcept override;

    private:
        ice::Allocator& _backing_allocator;
        ice::u32 _total_allocated{ 0 };

        VkAllocationCallbacks _vulkan_callbacks;
    };

    //auto allocation_callbacks_struct(VulkanAllocator& alloc) noexcept -> VkAllocationCallbacks;

} // namespace ice::render::vk
