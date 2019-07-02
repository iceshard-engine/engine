#pragma once
#include <core/data/view.hxx>
#include <core/allocator.hxx>

namespace core
{


    //! \brief This class defines an allocated data chunk which can be modified.
    //!
    //! \remarks The data will be released if the object goes out of scope.
    class data_chunk final
    {
    public:
        //! \brief A new data chunk object with the given size.
        data_chunk(core::allocator& alloc, uint32_t size) noexcept;

        //! \brief A new data chunk object from the given data and it's size.
        data_chunk(core::allocator& alloc, void* data, uint32_t size) noexcept;

        //! \brief A new data chunk object from the given data_view object.
        data_chunk(core::allocator& alloc, data_view data) noexcept;

        //! \brief Moves the data chunk to this object.
        data_chunk(data_chunk&& other) noexcept;

        //! \brief Copies the given data chunk.
        data_chunk(const data_chunk& other) noexcept;

        //! \brief Releases the allocated data.
        ~data_chunk() noexcept;


        //! \brief Moves the data chunk to this object.
        auto operator=(data_chunk&& other) noexcept->data_chunk&;

        //! \brief Copies the given data chunk.
        auto operator=(const data_chunk& other) noexcept->data_chunk&;

        //! \brief Returns a view into this data chunk.
        operator data_view() noexcept { return { _data, _size }; }


        //! \brief The chunk data.
        auto data() noexcept -> void* { return _data; }

        //! \brief The chunk data.
        auto data() const noexcept -> const void* { return _data; }

        //! \brief The chunk size.
        auto size() const noexcept { return _size; }


    private:
        //! \brief The associated allocator.
        core::allocator* _allocator;

        //! \brief The data location.
        void* _data;

        //! \brief The data size.
        uint32_t _size;
    };


} // namespace core
