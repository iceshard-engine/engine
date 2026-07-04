#include <ice/i18n_core_module.hxx>
#include <ice/i18n_string.hxx>

namespace ice
{

    ice::I18NString::I18NString(ice::String text) noexcept
        : _data{ text._data }
        , _data_offset{ 0 }
        , _size_offset{ static_cast<i32>(text.size().u32()) }
    {
    }

    ice::I18NString::I18NString(ice::I18NReference reference) noexcept
        : _data{ reference._data }
        , _data_offset{ reference.fallback_offset().u8() }
        , _size_offset{ static_cast<i32>(reference.fallback().size().u32()) }
    {
        ice::I18NCoreModule::resolve(*this, reference);
    }

    bool I18NString::resolve() noexcept
    {
        if (_size_offset >= 0 && _data_offset > 0)
        {
            ice::I18NReference const reference = ice::I18NReference::from_string({ static_cast<char const*>(_data), static_cast<ice::u32>(_data_offset) });
            ice::I18NCoreModule::resolve(*this, reference);
        }
        return _size_offset < 0;
    }

    auto I18NString::data() const noexcept -> ValueType*
    {
        if (_size_offset >= 0)
        {
            return static_cast<char const*>(ice::ptr_add(_data, ice::usize{static_cast<ice::u32>(_data_offset)}));
        }
        else
        {
            return *static_cast<char const* const*>(ice::ptr_add(_data, 8_B));
        }
    }

    auto I18NString::size() const noexcept -> SizeType
    {
        if (_size_offset >= 0)
        {
            return _size_offset;
        }
        else
        {
            return *static_cast<ice::ncount const*>(_data);
        }
    }

} // namespace ice
