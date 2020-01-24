#include "vulkan_allocator.hxx"
#include <core/memory.hxx>
#include <core/debug/assert.hxx>

namespace render::vulkan
{
    namespace detail
    {

        struct allocation_header
        {
            uint32_t requested_size;
            uint32_t requested_alignment;
        };

        //! \brief If we need to align the memory allocation we pad the header with this
        //!     value after storing the size. That way we can find the pointer header.
        //!
        //! \remarks The value is 4 bytes because thats the lowest possible alignment.
        static constexpr uint32_t HEADER_PAD_VALUE = 0xffffffffu;

        //! \brief Returns the data pointer from the given header.
        //!
        //! \param [in] header The allocation header.
        //! \param [in] align The alignment of the allocation.
        inline void* data_pointer(allocation_header* const header, uint32_t alignment) noexcept
        {
            return core::memory::utils::align_forward(header + 1, alignment);
        }

        //! \brief Returns the allocation header from the given data pointer.
        inline auto header(void* const data) noexcept -> allocation_header*
        {
            auto* temp_pointer = reinterpret_cast<uint32_t*>(data);

            // Subtract the pointer for every HEADER_PAD_VALUE we encounter.
            while (temp_pointer[-1] == HEADER_PAD_VALUE)
            {
                --temp_pointer;
            }

            // Return the pointer subtracted by the size of the allocation header.
            return reinterpret_cast<allocation_header*>(core::memory::utils::pointer_sub(temp_pointer, sizeof(allocation_header)));
        }

        // \brief Stores the size in the header and pads with HEADER_PAD_VALUE up to the data pointer.
        inline void fill(allocation_header* header, void* data_pointer, uint32_t requested_size, uint32_t requested_alignment) noexcept
        {
            header->requested_size = requested_size;
            header->requested_alignment = requested_alignment;

            auto* header_pointer = reinterpret_cast<uint32_t*>(header + 1);
            while (header_pointer < data_pointer)
            {
                *header_pointer = HEADER_PAD_VALUE;
                header_pointer += 1;
            }
        }

        auto vk_iceshard_allocate(void * userdata, size_t size, size_t alignment, VkSystemAllocationScope /*scope*/) noexcept -> void *
        {
            auto* const allocator = reinterpret_cast<render::vulkan::VulkanAllocator*>(userdata);
            return allocator->allocate(static_cast<uint32_t>(size), static_cast<uint32_t>(alignment));
        }

        auto vk_iceshard_reallocate(void * userdata, void * original, size_t size, size_t alignment, VkSystemAllocationScope /*scope*/) noexcept -> void *
        {
            auto* const allocator = reinterpret_cast<render::vulkan::VulkanAllocator*>(userdata);
            return allocator->reallocate(original, static_cast<uint32_t>(size), static_cast<uint32_t>(alignment));
        }

        void vk_iceshard_free(void * userdata, void * memory) noexcept
        {
            auto* const allocator = reinterpret_cast<render::vulkan::VulkanAllocator*>(userdata);
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

    VulkanAllocator::VulkanAllocator(core::allocator& backing_allocator) noexcept
        : _backing_allocator{ backing_allocator }
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
        IS_ASSERT(_total_allocated == 0, "Unreleased memory from the Vulkan renderer.");
    }

    //! \copydoc allocator::allocate(uint32_t, uint32_t) noexcept
    auto VulkanAllocator::allocate(uint32_t size, uint32_t align) noexcept -> void*
    {
        auto const total_size = static_cast<uint32_t>(sizeof(detail::allocation_header)) + size + align;

        // We use the alignment of the allocation_header so we don't waste data on the backing allocator, we already accounted for the alignment.
        void* result = _backing_allocator.allocate(total_size, static_cast<uint32_t>(alignof(detail::allocation_header)));

        _allocation_tracker[result] += 1;

        auto* header = reinterpret_cast<detail::allocation_header*>(result);
        auto* data_pointer = detail::data_pointer(header, align);
        IS_ASSERT(reinterpret_cast<uintptr_t>(data_pointer) % align == 0, "Invalid data pointer allignment for Vulkan allocation!");

        // We fill gaps between the header and the data pointer so we can later find the header easily.
        detail::fill(header, data_pointer, size, align);

        // Update the total allocated size.
        _total_allocated += header->requested_size;

        return data_pointer;
    }

    //! \brief #todo
    auto VulkanAllocator::reallocate(void* ptr, uint32_t size, uint32_t align) noexcept -> void*
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
                auto* header = detail::header(ptr);

                // Create a new allocation with the exact same alignment.
                // This requirement can be found on the following website (26.08.2019)
                // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/PFN_vkReallocationFunction.html
                if (auto* new_ptr = this->allocate(size, header->requested_alignment))
                {
                    // If we got a valid allocation, copy the data to the new pointer
                    // and release the old one afterwards.
                    memcpy(new_ptr, ptr, std::min(header->requested_size, size));

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
            auto* header = detail::header(ptr);
            _total_allocated -= header->requested_size;

            _allocation_tracker[header] -= 1;

            // We need to pass the header pointer because this pointer was returned by the backing allocator.
            _backing_allocator.deallocate(header);
        }
    }

    //! \copydoc allocator::allocated_size(void*) noexcept
    auto VulkanAllocator::allocated_size(void* ptr) noexcept -> uint32_t
    {
        uint32_t result = 0;
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

    //! \copydoc allocator::total_allocated() noexcept
    auto VulkanAllocator::total_allocated() noexcept -> uint32_t
    {
        return _total_allocated;
    }

} // namespace render::vulkan
