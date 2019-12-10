#pragma once
#include <core/base.hxx>

namespace core
{

    //! \brief This class defines a view into a chunk of raw data.
    struct data_view final
    {
        constexpr data_view() noexcept = default;

        //! \brief Creates a new view info the given data.
        constexpr data_view(void const* data_ptr, uint32_t sz) noexcept;

        //! \brief The chunk data.
        constexpr auto data() const noexcept { return _data; }

        //! \brief The chunk size.
        constexpr auto size() const noexcept { return _size; }

        //! \brief The data location.
        void const* _data = nullptr;

        //! \brief The data size.
        uint32_t _size = 0u;
    };

    inline constexpr data_view::data_view(void const* data_ptr, uint32_t sz) noexcept
        : _data{ data_ptr }
        , _size{ sz }
    {
    }

    //! \brief This class defines a view into a chunk of raw data.
    struct data_view_aligned final
    {
        data_view_aligned() noexcept = default;

        //! \brief Creates a new view info the given data.
        data_view_aligned(const void* data_ptr, uint32_t sz, uint32_t align) noexcept;

        //! \brief The chunk data.
        auto data() const noexcept { return _data; }

        //! \brief The chunk size.
        auto size() const noexcept { return _size; }

        //! \brief The chunk size.
        auto alignment() const noexcept { return _align; }

        //! \brief The data location.
        void const* _data = nullptr;

        //! \brief The data size.
        uint32_t _size = 0u;

        //! \brief The data alignment.
        uint32_t _align = 0u;
    };

} // namespace core
