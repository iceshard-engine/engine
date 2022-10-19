#include "vk_allocator.hxx"

namespace ice::render::vk
{
    namespace detail
    {

        struct AllocationHeader
        {
            ice::usize requested_size;
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
            header->requested_size = requested_size;
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
            VulkanAllocator* const allocator = reinterpret_cast<VulkanAllocator*>(userdata);
            return allocator->allocate({ { size }, static_cast<ice::ualign>(alignment) }).memory;
        }

        auto vk_iceshard_reallocate(void* userdata, void* original, size_t size, size_t alignment, VkSystemAllocationScope /*scope*/) noexcept -> void*
        {
            VulkanAllocator* const allocator = reinterpret_cast<VulkanAllocator*>(userdata);
            return allocator->do_reallocate(original, { { size }, static_cast<ice::ualign>(alignment) }).memory;
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
    }

    //! \brief #todo
    auto VulkanAllocator::do_reallocate(void* pointer, ice::AllocRequest req) noexcept -> ice::AllocResult
    {
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
                    memcpy(new_res.memory, pointer, ice::min(header->requested_size, req.size));

                    // Release the old pointer
                    this->deallocate(pointer);
                    result = new_res;
                }
            }
        }
        return result;
    }

    //! \copydoc allocator::deallocate(void*) noexcept
    void VulkanAllocator::do_deallocate(void* pointer) noexcept
    {
        // We need to pass the header pointer because this pointer was returned by the backing allocator.
        _backing_allocator.deallocate(detail::header(pointer));
    }

    auto VulkanAllocator::vulkan_callbacks() const noexcept -> VkAllocationCallbacks const*
    {
        return &_vulkan_callbacks;
    }

} // namespace ice::render::vk
