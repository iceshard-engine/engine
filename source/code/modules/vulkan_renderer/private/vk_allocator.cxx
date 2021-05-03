#include "vk_allocator.hxx"
#include <ice/memory/pointer_arithmetic.hxx>

namespace ice::render::vk
{
    namespace detail
    {

        struct AllocationHeader
        {
            ice::u32 requested_size;
            ice::u32 requested_alignment;
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
        inline void* data_pointer(AllocationHeader* const header, ice::u32 alignment) noexcept
        {
            return ice::memory::ptr_align_forward(header + 1, alignment);
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
            return reinterpret_cast<AllocationHeader*>(ice::memory::ptr_sub(temp_pointer, sizeof(AllocationHeader)));
        }

        // \brief Stores the size in the header and pads with HEADER_PAD_VALUE up to the data pointer.
        inline void fill(AllocationHeader* header, void* data_pointer, ice::u32 requested_size, ice::u32 requested_alignment) noexcept
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
            return allocator->allocate(static_cast<ice::u32>(size), static_cast<ice::u32>(alignment));
        }

        auto vk_iceshard_reallocate(void* userdata, void* original, size_t size, size_t alignment, VkSystemAllocationScope /*scope*/) noexcept -> void*
        {
            VulkanAllocator* const allocator = reinterpret_cast<VulkanAllocator*>(userdata);
            return allocator->reallocate(original, static_cast<ice::u32>(size), static_cast<ice::u32>(alignment));
        }

        void vk_iceshard_free(void* userdata, void* memory) noexcept
        {
            VulkanAllocator* const allocator = reinterpret_cast<VulkanAllocator*>(userdata);
            allocator->deallocate(memory);
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

    VulkanAllocator::VulkanAllocator(ice::Allocator& backing_allocator) noexcept
        : ice::Allocator{ backing_allocator }
        , _backing_allocator{ backing_allocator }
        , _allocation_tracker{ _backing_allocator }
    {
        _vulkan_callbacks.pUserData = this;
        _vulkan_callbacks.pfnAllocation = detail::vk_iceshard_allocate;
        _vulkan_callbacks.pfnReallocation = detail::vk_iceshard_reallocate;
        _vulkan_callbacks.pfnFree = detail::vk_iceshard_free;
        _vulkan_callbacks.pfnInternalAllocation = nullptr;
        _vulkan_callbacks.pfnInternalFree = nullptr;
    }

    VulkanAllocator::~VulkanAllocator() noexcept
    {
        // #todo assert
        //IS_ASSERT(_total_allocated == 0, "Unreleased memory from the Vulkan renderer.");
    }

    auto VulkanAllocator::allocate(ice::u32 size, ice::u32 align) noexcept -> void*
    {
        ice::u32 const total_size = static_cast<ice::u32>(sizeof(detail::AllocationHeader)) + size + align;

        // We use the alignment of the allocation_header so we don't waste data on the backing allocator, we already accounted for the alignment.
        void* result = _backing_allocator.allocate(total_size, static_cast<ice::u32>(alignof(detail::AllocationHeader)));

        _allocation_tracker[result] += 1;

        detail::AllocationHeader* header = reinterpret_cast<detail::AllocationHeader*>(result);
        void* data_pointer = detail::data_pointer(header, align);
        // #todo assert
        //IS_ASSERT(reinterpret_cast<uintptr_t>(data_pointer) % align == 0, "Invalid data pointer allignment for Vulkan allocation!");

        // We fill gaps between the header and the data pointer so we can later find the header easily.
        detail::fill(header, data_pointer, size, align);

        // Update the total allocated size.
        _total_allocated += header->requested_size;

        return data_pointer;
    }

    //! \brief #todo
    auto VulkanAllocator::reallocate(void* ptr, ice::u32 size, ice::u32 align) noexcept -> void*
    {
        // We will return a nullptr when deallocating, as there was not indication what to return during deallocations.
        void* result = nullptr;
        if (size == 0)
        {
            this->deallocate(ptr);
        }
        else
        {
            if (ptr == nullptr)
            {
                result = this->allocate(size, align);
            }
            else // original != nullptr
            {
                detail::AllocationHeader* header = detail::header(ptr);

                // Create a new allocation with the exact same alignment.
                // This requirement can be found on the following website (26.08.2019)
                // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/PFN_vkReallocationFunction.html
                if (auto* new_ptr = this->allocate(size, header->requested_alignment))
                {
                    // If we got a valid allocation, copy the data to the new pointer
                    // and release the old one afterwards.
                    memcpy(new_ptr, ptr, ice::min(header->requested_size, size));

                    // Release the old pointer
                    this->deallocate(ptr);
                    result = new_ptr;
                }
            }
        }
        return result;
    }

    //! \copydoc allocator::deallocate(void*) noexcept
    void VulkanAllocator::deallocate(void* ptr) noexcept
    {
        if (ptr != nullptr)
        {
            detail::AllocationHeader* header = detail::header(ptr);
            _total_allocated -= header->requested_size;

            _allocation_tracker[header] -= 1;

            // We need to pass the header pointer because this pointer was returned by the backing allocator.
            _backing_allocator.deallocate(header);
        }
    }

    //! \copydoc allocator::allocated_size(void*) noexcept
    auto VulkanAllocator::allocated_size(void* ptr) const noexcept -> ice::u32
    {
        ice::u32 result = 0;
        if (ptr != nullptr)
        {
            result = detail::header(ptr)->requested_size;
        }
        return result;
    }

    auto VulkanAllocator::vulkan_callbacks() const noexcept -> VkAllocationCallbacks const*
    {
        return &_vulkan_callbacks;
    }

    auto VulkanAllocator::backing_allocator() const noexcept -> ice::Allocator&
    {
        return _backing_allocator;
    }

    //! \copydoc allocator::total_allocated() noexcept
    auto VulkanAllocator::total_allocated() const noexcept -> ice::u32
    {
        return _total_allocated;
    }

} // namespace ice::render::vk
