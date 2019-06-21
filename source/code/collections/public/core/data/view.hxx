#pragma once
#include <core/base.hxx>

namespace core
{


//! \brief This class defines a view into a chunk of raw data.
struct data_view final
{
    //! \brief Creates a new view info the given data.
    data_view(const void* data, uint32_t size) noexcept;


    //! \brief The chunk data.
    auto data() noexcept { return _data; }

    //! \brief The chunk size.
    auto size() noexcept { return _size; }


    //! \brief The data location.
    const void* const _data;

    //! \brief The data size.
    const uint32_t _size;
};


} // namespace core
