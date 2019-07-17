#pragma once
#include <core/allocator.hxx>
#include <core/allocators/scratch_allocator.hxx>
#include <core/data/view.hxx>

namespace core
{


    //! \brief This class defines a raw data object, which will grow if needed.
    struct Buffer
    {
        //! \brief Creates a new empty Buffer object.
        Buffer(core::allocator& alloc) noexcept;

        //! \brief Creates a new Buffer object with the initial data copied from the given data view.
        Buffer(core::allocator& alloc, data_view data) noexcept;

        //! \brief Creates a new Buffer object from the other.
        //! \details The allocator used will be the same, and the contents will be copied.
        Buffer(const Buffer& other) noexcept;

        //! \brief Releases the allocated data.
        ~Buffer();


        //! \brief Replaces the Buffer contents from the other object.
        auto operator=(const Buffer& other) noexcept -> Buffer&;

        //! \brief Returns a view into this data buffer.
        operator data_view() noexcept { return { _data, _size }; }


        //! \brief The associated allocator.
        core::allocator* _allocator;

        //! \brief The current size.
        uint32_t _size{ 0 };

        //! \brief The current capacity.
        uint32_t _capacity{ 0 };

        //! \brief The data.
        void* _data{ nullptr };
    };


    //! \brief Functions to manipulate a Buffer object.
    namespace buffer
    {

        //! \brief The current size of the data.
        auto size(const Buffer& b) noexcept -> uint32_t;

        //! \brief The current capacity of the buffer.
        auto capacity(const Buffer& b) noexcept -> uint32_t;

        //! \brief Checks if the Buffer is empty.
        bool empty(const Buffer& b) noexcept;

        //! \brief The buffer data pointer.
        auto data(const Buffer& b) noexcept -> void*;

        //! \brief Appends data to the buffer
        void append(Buffer& b, const void* data, uint32_t size) noexcept;

        //! \brief Appends data to the buffer
        void append(Buffer& b, data_view data) noexcept;

        //! \brief Changes the size of the buffer
        //! \remarks Does not reallocate memory unless necessary.
        void resize(Buffer& a, uint32_t new_size) noexcept;

        //! \brief Removes all items in the buffer
        //! \remarks Does not free memory.
        void clear(Buffer& b) noexcept;

        //! \brief Reallocates the buffer to the specified capacity.
        void set_capacity(Buffer& b, uint32_t new_capacity) noexcept;

        //! \brief  Grows the buffer using a geometric progression formula.
        //!
        //! \details If a min_capacity is specified, the buffer will grow
        //!     to at least that capacity.
        void grow(Buffer& b, uint32_t min_capacity = 0) noexcept;

        //! \brief Ensures the specified capacity is available.
        void reserve(Buffer& b, uint32_t new_capacity) noexcept;

        //! \brief Trims the buffer so that its capacity matches its size.
        //! \remarks If the buffer size is 0, it will just release the data.
        void trim(Buffer& b) noexcept;

        //! \brief Beginning of the buffer.
        auto begin(const Buffer& b) noexcept -> const void*;

        //! \brief Beginning of the buffer.
        auto begin(Buffer& b) noexcept -> void*;

        //! \brief End of the buffer.
        auto end(const Buffer& b) noexcept -> const void*;

        //! \brief End of the buffer.
        auto end(Buffer& b) noexcept -> void*;

    } // namespace buffer


} // namespace core
