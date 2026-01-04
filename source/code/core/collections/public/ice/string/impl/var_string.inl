/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT


namespace ice
{

    namespace string
    {

#if 0
        inline auto size(ice::string::VarStringType auto const& str) noexcept -> ice::ucount
        {
            return ice::string::detail::read_varstring_size(str._data);
        }

        inline auto capacity(ice::string::VarStringType auto const& str) noexcept -> ice::ucount
        {
            return ice::string::size(str);
        }

        inline bool empty(ice::string::VarStringType auto const& str) noexcept
        {
            return str._data == nullptr || str._data[0] == '\0';
        }

        template<ice::string::VarStringType StringType>
        inline auto begin(StringType const& str) noexcept -> typename StringType::ConstIterator
        {
            return ice::string::detail::data_varstring(str._data);
        }

        template<ice::string::VarStringType StringType>
        inline auto end(StringType const& str) noexcept -> typename StringType::ConstIterator
        {
            ice::ucount bytes = 0;
            ice::ucount const size = ice::string::detail::read_varstring_size(str._data, bytes);
            return str._data + bytes + size;
        }
#endif

#if 0
        inline auto data_view(ice::string::VarStringType auto const& str) noexcept -> ice::Data
        {
            ice::ucount bytes = 0;
            ice::ucount const size = ice::string::detail::read_varstring_size(str._data, bytes);

            return {
                .location = str._data,
                .size = { size + bytes },
                .alignment = ice::ualign::b_1
            };
        }

        auto serialize(ice::string::VarStringType auto const& str, ice::Memory target) noexcept -> ice::Memory
        {
            ice::ucount const size = ice::string::size(str);
            ICE_ASSERT_CORE(
                ice::string::detail::calc_varstring_required_size(size) <= target.size.value
            );

            ice::ucount const sizebytes = ice::string::detail::write_varstring_size(target.location, size);
            target.location = ice::ptr_add(target.location, ice::usize{ sizebytes });
            target.size.value -= sizebytes;

            ice::memcpy(target.location, str._data + sizebytes, size);
            target.location = ice::ptr_add(target.location, ice::usize{ size });
            target.size.value -= size;
            target.alignment = ice::ualign::b_1;
            return target;
        }
#endif

    } // namespace string

#if 0
    namespace data
    {

        template<typename CharType>
        inline auto read_varstring(ice::Data data, ice::VarStringBase<CharType>& out_str) noexcept -> ice::Data
        {
            ICE_ASSERT_CORE(data.size >= 2_B); // 1 byte for size + 1 for a single character

            char const* const rawstr = reinterpret_cast<char const*>(data.location);

            ice::ucount bytes;
            ice::ucount const size = ice::string::detail::read_varstring_size(rawstr, bytes);
            if (size > 0)
            {
                out_str._data = rawstr;
            }

            return ice::ptr_add(data, { bytes + size });
        }

    } // namespace data
#endif

} // namespace ice
