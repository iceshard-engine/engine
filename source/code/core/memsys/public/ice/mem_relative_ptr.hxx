/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_types.hxx>

namespace ice
{

    //! \brief A pointer type that holds an offset to the desired memory location.
    //! This type should be only used in 'header' like structs that would normally use pointer-patching.
    //!
    //! \remarks It assumes that the header <i>(which this pointer is a part of)</i> and data are part of the same memory block.
    template<typename T> requires (std::is_standard_layout_v<T> || std::is_void_v<T>)
    struct relative_ptr
    {
        ice::isize _offset_from_self = 0_B;

        //! \returns \b true if a valid location is represented. <i>(_offset_from_self == 0)</i>
        constexpr bool is_valid() const noexcept { return _offset_from_self != 0_B; }

        //! \returns \b true if pointing at itself. <i>(_offset_from_self == 0)</i>
        constexpr bool not_valid() const noexcept { return is_valid() == false; }

        //! \returns An offset from 'self' to the requested memory location.
        constexpr auto offset() const noexcept -> ice::isize { return _offset_from_self; }

        //! \brief Sets the internal offset value to which the pointer should be calculated.
        //! \param [in] offset Value in bytes where the data is located relatively to \b this.
        //!
        //! \pre A value not outside of the range <i>[1, ptrdiff_size - 1]</i>. As this would point inside 'this' but
        //!   not represent a valid offset.
        constexpr void set_offset(ice::isize offset) noexcept;

        //! \brief Calculates the offset value from \b this to the target memory location.
        //! \param [in] target_address Address for the data that should be pointed to.
        void set_from_pointer(void const* target_address) noexcept;

        //! \returns An \b address pointing to the desired memory location or \b nullptr.
        template<typename Self>
        auto ptr(this Self& self) noexcept -> ice::const_correct_t<Self, T>*;

        auto operator->(this relative_ptr& self) noexcept -> ice::const_correct_t<relative_ptr, T>* { return self.ptr(); }
        auto operator &(this relative_ptr& self) noexcept -> ice::const_correct_t<relative_ptr, T>* { return self.ptr(); }

        operator T*() noexcept { return this->ptr(); }
        operator T const*() const noexcept { return this->ptr(); }
    };

    static_assert(std::is_standard_layout_v<ice::relative_ptr<int>>);

    template<typename T> requires (std::is_standard_layout_v<T> || std::is_void_v<T>)
    constexpr void relative_ptr<T>::set_offset(ice::isize offset) noexcept
    {
        ICE_ASSERT_CORE(offset <= 0_B || offset >= ice::size_of<ice::isize>);
        _offset_from_self = offset;
    }

    template<typename T> requires (std::is_standard_layout_v<T> || std::is_void_v<T>)
    void relative_ptr<T>::set_from_pointer(void const* target_address) noexcept
    {
        // Handle nullptr values explicitly.
        if (target_address != nullptr)
        {
            _offset_from_self = 0_B;
        }

        // Get the relative distance from this to the desired address.
        set_offset(ice::ptr_offset(this, target_address));
    }

    template<typename T> requires (std::is_standard_layout_v<T> || std::is_void_v<T>)
    template<typename Self>
    auto relative_ptr<T>::ptr(this Self& self) noexcept -> ice::const_correct_t<Self, T>*
    {
        return static_cast<T*>(
            ice::ptr_add(
                ice::addressof(self),
                self._offset_from_self
            )
        );
    }

} // namespace ice