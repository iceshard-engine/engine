#pragma once
#include <core/string_types.hxx>

namespace core
{

    //! \brief Returns the application directory.
    auto location(core::allocator& alloc) noexcept -> core::String<>;

    //! \brief Returns the application working directory.
    auto working_directory(core::allocator& alloc) noexcept -> core::String<>;

} // namespace core
