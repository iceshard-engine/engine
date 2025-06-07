/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "vk_allocator.hxx"

namespace ice::render::vk
{
    namespace detail
    {

        struct AllocationHeader
        {
            ice::u32 requested_size;
            ice::ualign requested_alignment;
        };

        //! \brief If we need to align the memory allocation we pad the header with this
        //!     value after storing the size. That way we can find the pointer header.
        //!
        //! \remarks The value is 4 bytes because thats the lowest possible alignment.
        static constexpr ice::u32 Constant_HeaderPadValue = 0xffffffffu;

        //! \brief Returns the data pointer from the given header.
        //!
        //! \param [in] header The allocation header.
        //! \param [in] align The alignment of the allocation.
        inline void* data_pointer(AllocationHeader* const header, ice::ualign alignment) noexcept
        {
            return ice::align_to(header + 1, alignment).value;
        }

        //! \brief Returns the allocation header from the given data pointer.
        inline auto header(void* const data) noexcept -> AllocationHeader*
        {
            ice::u32* temp_pointer = reinterpret_cast<ice::u32*>(data);

            // Subtract the pointer for every HEADER_PAD_VALUE we encounter.
            while (temp_pointer[-1] == Constant_HeaderPadValue)
            {
                --temp_pointer;
            }

            // Return the pointer subtracted by the size of the allocation header.
            return reinterpret_cast<AllocationHeader*>(ice::ptr_sub(temp_pointer, ice::size_of<AllocationHeader>));
        }

        // \brief Stores the size in the header and pads with HEADER_PAD_VALUE up to the data pointer.
        inline void fill(AllocationHeader* header, void* data_pointer, ice::usize requested_size, ice::ualign requested_alignment) noexcept
        {
            header->requested_size = ice::u32(requested_size.value);
            header->requested_alignment = requested_alignment;

            ice::u32* header_pointer = reinterpret_cast<ice::u32*>(header + 1);
            while (header_pointer < data_pointer)
            {
                *header_pointer = Constant_HeaderPadValue;
                header_pointer += 1;
            }
        }

        auto vk_iceshard_allocate(void* userdata, size_t size, size_t alignment, VkSystemAllocationScope /*scope*/) noexcept -> void*
        {
            // if (size == 0) return nullptr;

            VulkanAllocator* const allocator = reinterpret_cast<VulkanAllocator*>(userdata);
            AllocResult const res = allocator->allocate({ { size }, static_cast<ice::ualign>(alignment) });
            return res.memory;
        }

        auto vk_iceshard_reallocate(void* userdata, void* original, size_t size, size_t alignment, VkSystemAllocationScope /*scope*/) noexcept -> void*
        {
            VulkanAllocator* const allocator = reinterpret_cast<VulkanAllocator*>(userdata);
            AllocResult const res = allocator->do_reallocate(original, { { size }, static_cast<ice::ualign>(alignment) });
            return res.memory;
        }

        void vk_iceshard_free(void* userdata, void* pointer) noexcept
        {
            if (pointer == nullptr) return;

            VulkanAllocator* const allocator = reinterpret_cast<VulkanAllocator*>(userdata);
            allocator->deallocate(pointer);
        }

        //void vk_iceshard_internal_allocate_event(void* userdata, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope) noexcept
        //{
        //    PFN_vkInternalAllocationNotification f;
        //}

        //void vk_iceshard_internal_free_event(void* userdata, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope) noexcept
        //{
        //    PFN_vkInternalFreeNotification f;
        //}

    } // namespace detail

    VulkanAllocator::VulkanAllocator(
        ice::Allocator& backing_allocator,
        std::source_location src_loc
    ) noexcept
        : ice::Allocator{ src_loc, backing_allocator, "vulkan-alloc" }
        , _backing_allocator{ backing_allocator }
    {
        _vulkan_callbacks.pUserData = this;
        _vulkan_callbacks.pfnAllocation = detail::vk_iceshard_allocate;
        _vulkan_callbacks.pfnReallocation = detail::vk_iceshard_reallocate;
        _vulkan_callbacks.pfnFree = detail::vk_iceshard_free;
        _vulkan_callbacks.pfnInternalAllocation = nullptr;
        _vulkan_callbacks.pfnInternalFree = nullptr;
    }

    VulkanAllocator::~VulkanAllocator() noexcept = default;

    auto VulkanAllocator::do_allocate(ice::AllocRequest request) noexcept -> ice::AllocResult
    {
#if 0
        request.alignment = ice::max(request.alignment, ice::ualign::b_4);
        ICE_ASSERT_CORE(request.alignment >= ice::ualign::b_4);

        ice::meminfo subreq = ice::meminfo_of<detail::AllocationHeader>;
        ice::usize const offset_data = subreq += ice::meminfo{ request.size, request.alignment };

        // We use the alignment of the allocation_header so we don't waste data on the backing allocator, we already accounted for the alignment.
        ice::AllocResult const result = _backing_allocator.allocate(subreq);

        detail::AllocationHeader* header = reinterpret_cast<detail::AllocationHeader*>(result.memory);
        void* data_pointer = ice::ptr_add(result.memory, offset_data);

        // We fill gaps between the header and the data pointer so we can later find the header easily.
        detail::fill(header, data_pointer, request.size, request.alignment);

        return AllocResult{
            .memory = data_pointer,
            .size = request.size,
            .alignment = request.alignment
        };
#else
        ICE_ASSERT_CORE(request.alignment <= ice::ualign::b_8);
        request.alignment = ice::ualign::b_8;
        request.size = ice::align_to(request.size, request.alignment).value + 8_B;

        ice::AllocResult result = _backing_allocator.allocate(request);
        ice::u32* header = reinterpret_cast<ice::u32*>(result.memory);
        result.memory = ice::ptr_add(result.memory, 8_B);
        result.size.value -= 8;
        header[0] = ice::u32(result.size.value);
        header[1] = ice::u32(request.alignment);
        return result;
#endif
    }

    //! \brief #todo
    auto VulkanAllocator::do_reallocate(void* pointer, ice::AllocRequest req) noexcept -> ice::AllocResult
    {
#if 0
        // We will return a nullptr when deallocating, as there was not indication what to return during deallocations.
        ice::AllocResult result{ };
        if (req.size == 0_B)
        {
            this->deallocate(pointer);
        }
        else
        {
            if (pointer == nullptr)
            {
                result = this->allocate(req);
            }
            else // original != nullptr
            {
                detail::AllocationHeader* header = detail::header(pointer);

                // Create a new allocation with the exact same alignment.
                // This requirement can be found on the following website (26.08.2019)
                // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/PFN_vkReallocationFunction.html
                if (ice::AllocResult new_res = this->allocate({ req.size, header->requested_alignment }); new_res.memory != nullptr)
                {
                    // If we got a valid allocation, copy the data to the new pointer
                    // and release the old one afterwards.
                    memcpy(new_res.memory, pointer, ice::min(ice::usize{ header->requested_size }, req.size));

                    // Release the old pointer
                    this->deallocate(pointer);
                    result = new_res;
                }
            }
        }
        return result;
#else
        // We will return a nullptr when deallocating, as there was not indication what to return during deallocations.
        ice::AllocResult result{ };
        if (req.size == 0_B)
        {
            this->deallocate(pointer);
        }
        else
        {
            if (pointer == nullptr)
            {
                result = this->allocate(req);
            }
            else // original != nullptr
            {
                ice::u32 const* header = reinterpret_cast<ice::u32*>(ice::ptr_sub(pointer, 8_B));
                ice::usize const size{ header[0] };
                ice::ualign const align{ ice::ualign(header[1]) };

                ICE_ASSERT_CORE(req.alignment <= align);

                ice::AllocResult new_res = this->allocate({ req.size, align });
                ICE_ASSERT_CORE(new_res.memory != nullptr);
                ice::memcpy(new_res.memory, pointer, ice::min(ice::usize{ size }, req.size));

                this->deallocate(pointer);
                result = new_res;
            }
        }
        return result;
#endif
    }

    //! \copydoc allocator::deallocate(void*) noexcept
    void VulkanAllocator::do_deallocate(void* pointer) noexcept
    {
#if 0
        // We need to pass the header pointer because this pointer was returned by the backing allocator.
        _backing_allocator.deallocate(detail::header(pointer));
#else
        ice::u32 const* header = reinterpret_cast<ice::u32*>(ice::ptr_sub(pointer, 8_B));

        _backing_allocator.deallocate((void*)header);
#endif
    }

    auto VulkanAllocator::vulkan_callbacks() const noexcept -> VkAllocationCallbacks const*
    {
#if 0 // Disable until we actually have the time to fix this
        return &_vulkan_callbacks;
#else
        return nullptr;
#endif
    }

} // namespace ice::render::vk
