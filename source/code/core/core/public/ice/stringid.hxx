#pragma once
#include <ice/base.hxx>
#include <ice/hash.hxx>

namespace ice
{

    namespace detail::stringid_type_v2
    {

        // #TODO: Move ths to a proper 'settings' file at some point
        static constexpr bool use_stringid_debug_implementation = ice::build::is_debug || ice::build::is_develop;

        enum class StringID_Hash : uint64_t
        {
            Invalid = 0x0
        };

        template<bool DebugImpl>
        struct StringID;

        template<>
        struct StringID<false>
        {
            static constexpr bool has_debug_fields = false;

            StringID_Hash hash_value;
        };

        template<>
        struct StringID<true>
        {
            static constexpr bool has_debug_fields = true;

            StringID_Hash hash_value;

            char hash_origin[24];
        };


        template<bool DebugImpl = false>
        constexpr auto stringid(std::string_view value) noexcept -> StringID<DebugImpl>;

        template<>
        constexpr auto stringid<false>(std::string_view value) noexcept -> StringID<false>;

        template<>
        constexpr auto stringid<true>(std::string_view value) noexcept -> StringID<true>;


        constexpr auto origin_value(StringID<false> const&) noexcept -> std::string_view;

        constexpr auto origin_value(StringID<true> const&) noexcept -> std::string_view;

        template<bool DebugImpl>
        constexpr bool has_debug_fields(StringID<DebugImpl> const&) noexcept;


        template<bool DebugImpl>
        struct TypePicker
        {
            using StringID = StringID<DebugImpl>;

            using StringID_Arg = StringID;

            using StringID_Hash = StringID_Hash;
        };

        template<>
        struct TypePicker<true>
        {
            using StringID = StringID<true>;

            using StringID_Arg = StringID const&;

            using StringID_Hash = StringID_Hash;
        };

        constexpr bool operator==(
            typename TypePicker<true>::StringID_Arg left,
            typename TypePicker<true>::StringID_Arg right
        ) noexcept;

        constexpr bool operator!=(
            typename TypePicker<true>::StringID_Arg left,
            typename TypePicker<true>::StringID_Arg right
        ) noexcept;

        constexpr bool operator==(
            typename TypePicker<false>::StringID_Arg left,
            typename TypePicker<false>::StringID_Arg right
        ) noexcept;

        constexpr bool operator!=(
            typename TypePicker<false>::StringID_Arg left,
            typename TypePicker<false>::StringID_Arg right
        ) noexcept;

        using StringID_Types = TypePicker<use_stringid_debug_implementation>;

    } // namespace detail::stringid_type_v2


    using StringID = detail::stringid_type_v2::StringID_Types::StringID;

    using StringID_Arg = detail::stringid_type_v2::StringID_Types::StringID_Arg;

    using StringID_Hash = detail::stringid_type_v2::StringID_Types::StringID_Hash;

    static constexpr StringID stringid_invalid = StringID{ StringID_Hash{ 0 } };


    template<>
    constexpr auto hash(StringID_Arg value) noexcept -> uint64_t;

    constexpr auto stringid(std::string_view value) noexcept -> ice::StringID;

    constexpr auto stringid_hint(StringID_Arg value) noexcept -> std::string_view;

    constexpr auto stringid_hash(StringID_Arg value) noexcept -> ice::StringID_Hash;

    constexpr auto stringid_hash(StringID_Hash value) noexcept -> ice::StringID_Hash;

    constexpr auto operator""_sid(const char* cstr, size_t length) noexcept -> ice::StringID;

    constexpr auto operator""_sid_hash(const char* cstr, size_t length) noexcept -> ice::StringID_Hash;


    namespace detail::stringid_type_v2
    {

        template<>
        constexpr auto stringid<false>(std::string_view value) noexcept -> StringID<false>
        {
            return StringID<false> {
                .hash_value = StringID_Hash{
                    ice::detail::murmur2_hash::cexpr_murmur2_x64_64(value, 0xDA864239).h[0]
                },
            };
        }

        template<>
        constexpr auto stringid<true>(std::string_view value) noexcept -> StringID<true>
        {
            StringID<true> result{
                .hash_value = StringID_Hash{
                    ice::detail::murmur2_hash::cexpr_murmur2_x64_64(value, 0xDA864239).h[0]
                },
            };

            int32_t const cstr_size = static_cast<int32_t>(value.size());
            int32_t const origin_size = static_cast<int32_t>(std::size(result.hash_origin));

            int32_t const copy_count = std::min(origin_size, cstr_size);
            int32_t const copy_offset = std::max(0, cstr_size - copy_count);

            int64_t i = 0;
            for (auto& v : result.hash_origin)
            {
                if (i < copy_count)
                {
                    v = value[copy_offset + i];
                }
                i++;
            }

            if (copy_offset > 0)
            {
                result.hash_origin[0] = '~';
            }

            return result;
        }

        constexpr auto origin_value(StringID<false> const&) noexcept -> std::string_view
        {
            return {};
        }

        constexpr auto origin_value(StringID<true> const& value) noexcept -> std::string_view
        {
            return value.hash_origin;
        }

        template<bool DebugImpl>
        constexpr bool has_debug_fields(StringID<DebugImpl> const&) noexcept
        {
            return StringID<DebugImpl>::has_debug_fields;
        }


        constexpr bool operator==(
            typename TypePicker<true>::StringID_Arg left,
            typename TypePicker<true>::StringID_Arg right
        ) noexcept
        {
            return left.hash_value == right.hash_value;
        }

        constexpr bool operator!=(
            typename TypePicker<true>::StringID_Arg left,
            typename TypePicker<true>::StringID_Arg right
        ) noexcept
        {
            return !(left == right);
        }

        constexpr bool operator==(
            typename TypePicker<false>::StringID_Arg left,
            typename TypePicker<false>::StringID_Arg right
        ) noexcept
        {
            return left.hash_value == right.hash_value;
        }

        constexpr bool operator!=(
            typename TypePicker<false>::StringID_Arg left,
            typename TypePicker<false>::StringID_Arg right
        ) noexcept
        {
            return !(left == right);
        }

    } // namespace detail::stringid_type_v2

    constexpr auto stringid(std::string_view value) noexcept -> ice::StringID
    {
        return ice::detail::stringid_type_v2::stringid<ice::detail::stringid_type_v2::use_stringid_debug_implementation>(value);
    }

    constexpr auto stringid_hint(StringID_Arg value) noexcept -> std::string_view
    {
        return ice::detail::stringid_type_v2::origin_value(value);
    }

    constexpr auto stringid_hash(StringID_Hash value) noexcept -> ice::StringID_Hash
    {
        return value;
    }

    constexpr auto stringid_hash(StringID_Arg value) noexcept -> ice::StringID_Hash
    {
        return value.hash_value;
    }

    constexpr auto hash(StringID_Arg value) noexcept -> uint64_t
    {
        return static_cast<uint64_t>(value.hash_value);
    }

    constexpr auto operator""_sid(const char* cstr, size_t length) noexcept -> ice::StringID
    {
        return stringid({ cstr, length });
    }

    constexpr auto operator""_sid_hash(const char* cstr, size_t length) noexcept -> ice::StringID_Hash
    {
        return stringid({ cstr, length }).hash_value;
    }

} // namespace ice

//
//using core::operator""_sid;
//using core::operator""_sid_hash;
//
//template<bool debug_fields>
//struct fmt::formatter<core::cexpr::stringid_base_type<debug_fields>>
//{
//    using stringid_arg_type = typename core::cexpr::stringid_defined_types<debug_fields>::stringid_arg_type;
//
//    template<typename ParseContext>
//    constexpr auto parse(ParseContext& ctx)
//    {
//        return ctx.begin();
//    }
//
//    template<typename FormatContext>
//    constexpr auto format(stringid_arg_type value, FormatContext& ctx)
//    {
//        if (value == core::stringid_invalid)
//        {
//            return fmt::format_to(ctx.out(), "[sid:<invalid>]");
//        }
//        else
//        {
//            if constexpr (debug_fields == false)
//            {
//                return fmt::format_to(ctx.out(), "[sid:{:16x}]", core::hash(value));
//            }
//            else
//            {
//                return fmt::format_to(ctx.out(), "[sid:{:16x}]'{}'", core::hash(value), value.hash_origin);
//            }
//        }
//    }
//};
