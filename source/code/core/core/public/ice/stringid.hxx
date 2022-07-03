#pragma once
#include <ice/base.hxx>
#include <ice/hash.hxx>
#include <string_view>

namespace ice
{

    namespace detail::stringid_type_v2
    {

        // #TODO: Move ths to a proper 'settings' file at some point
        static constexpr bool use_stringid_debug_implementation = ice::build::is_debug || ice::build::is_develop;

        enum class StringID_Hash : ice::u64
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

            union
            {
                char hash_runtime_origin_hint[24];

                struct
                {
                    const char* hash_consteval_origin;
                    char has_consteval_value;
                };
            };
        };

        static_assert(sizeof(StringID<true>) == 32);

        template<bool DebugImpl = false>
        constexpr auto stringid(std::string_view value) noexcept -> StringID<DebugImpl>;

        template<bool DebugImpl = false>
        constexpr auto stringid(std::u8string_view value) noexcept -> StringID<DebugImpl>;

        template<>
        constexpr auto stringid<false>(std::string_view value) noexcept -> StringID<false>;

        template<>
        constexpr auto stringid<true>(std::string_view value) noexcept -> StringID<true>;

        template<>
        constexpr auto stringid<true>(std::u8string_view value) noexcept -> StringID<true>;

        constexpr auto origin_value(StringID<false> const&) noexcept -> std::string_view;

        constexpr auto origin_value(StringID<true> const&) noexcept -> std::string_view;

        template<bool DebugImpl>
        constexpr bool has_debug_fields(StringID<DebugImpl> const&) noexcept;


        template<bool DebugImpl>
        struct TypePicker
        {
            using StringID_Type = StringID<DebugImpl>;

            using StringID_Arg = StringID_Type;

            using StringID_Hash_Type = StringID_Hash;
        };

        template<>
        struct TypePicker<true>
        {
            using StringID_Type = StringID<true>;

            using StringID_Arg = StringID_Type const&;

            using StringID_Hash_Type = StringID_Hash;
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


    using StringID = detail::stringid_type_v2::StringID_Types::StringID_Type;

    using StringID_Arg = detail::stringid_type_v2::StringID_Types::StringID_Arg;

    using StringID_Hash = detail::stringid_type_v2::StringID_Types::StringID_Hash_Type;

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

        template<bool DebugImpl>
        constexpr auto stringid(std::u8string_view value) noexcept -> StringID<DebugImpl>
        {
            ice::detail::murmur2_hash::mm2_x64_64 const hash_result = ice::detail::murmur2_hash::cexpr_murmur2_x64_64(value, 0xDA864239);

            return StringID<DebugImpl> {
                .hash_value = StringID_Hash{
                    hash_result.h[0]
                },
            };
        }

        template<>
        constexpr auto stringid<false>(std::string_view value) noexcept -> StringID<false>
        {
            ice::detail::murmur2_hash::mm2_x64_64 const hash_result = ice::detail::murmur2_hash::cexpr_murmur2_x64_64(value, 0xDA864239);

            return StringID<false> {
                .hash_value = StringID_Hash{
                    hash_result.h[0]
                },
            };
        }

        template<>
        constexpr auto stringid<true>(std::string_view value) noexcept -> StringID<true>
        {
            ice::detail::murmur2_hash::mm2_x64_64 const hash_result = ice::detail::murmur2_hash::cexpr_murmur2_x64_64(value, 0xDA864239);

            StringID<true> result{
                .hash_value = StringID_Hash{
                    hash_result.h[0]
                },
            };

            if (std::is_constant_evaluated())
            {
                result.hash_consteval_origin = value.data();
                result.has_consteval_value = ~char{ 0 };
            }
            else
            {
                ice::i32 const cstr_size = static_cast<ice::i32>(value.size());
                ice::i32 const origin_size = static_cast<ice::i32>(std::size(result.hash_runtime_origin_hint));

                ice::i32 const copy_count = std::min(origin_size, cstr_size);
                ice::i32 const copy_offset = std::max(0, cstr_size - copy_count);



                ice::i32 i = 0;
                for (auto& v : result.hash_runtime_origin_hint)
                {
                    if (i < copy_count)
                    {
                        v = value[copy_offset + i];
                    }
                    else
                    {
                        v = char{};
                    }

                    i += 1;
                }

                if (copy_offset > 0)
                {
                    result.hash_runtime_origin_hint[0] = '~';
                }
            }

            return result;
        }

        template<>
        constexpr auto stringid<true>(std::u8string_view value) noexcept -> StringID<true>
        {
            ice::detail::murmur2_hash::mm2_x64_64 const hash_result = ice::detail::murmur2_hash::cexpr_murmur2_x64_64(value, 0xDA864239);

            StringID<true> result{
                .hash_value = StringID_Hash{
                    hash_result.h[0]
                },
            };

            if (std::is_constant_evaluated())
            {
                result.hash_consteval_origin = "utf8-not-supported";// value.data();
                result.has_consteval_value = ~char{ 0 };
            }
            else
            {
                ice::i32 const cstr_size = static_cast<ice::i32>(value.size());
                ice::i32 const origin_size = static_cast<ice::i32>(std::size(result.hash_runtime_origin_hint));

                ice::i32 const copy_count = std::min(origin_size, cstr_size);
                ice::i32 const copy_offset = std::max(0, cstr_size - copy_count);



                ice::i32 i = 0;
                for (auto& v : result.hash_runtime_origin_hint)
                {
                    if (i < copy_count)
                    {
                        v = static_cast<char>(value[copy_offset + i]);
                    }
                    else
                    {
                        v = char{};
                    }

                    i += 1;
                }

                if (copy_offset > 0)
                {
                    result.hash_runtime_origin_hint[0] = '~';
                }
            }

            return result;
        }

        constexpr auto origin_value(StringID<false> const&) noexcept -> std::string_view
        {
            return {};
        }

        constexpr auto origin_value(StringID<true> const& value) noexcept -> std::string_view
        {
            if (std::is_constant_evaluated())
            {
                return value.hash_consteval_origin;
            }
            else if (value.has_consteval_value == ~char{ 0 })
            {
                return value.hash_consteval_origin;
            }
            else
            {
                ice::u32 length = 24;
                while (length > 0 && value.hash_runtime_origin_hint[length-1] == 0)
                {
                    length -= 1;
                }

                return std::string_view{ value.hash_runtime_origin_hint, length };
            }
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

    constexpr auto stringid(std::u8string_view value) noexcept -> ice::StringID
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

    constexpr auto operator""_sid(const char8_t* cstr, size_t length) noexcept -> ice::StringID
    {
        return stringid({ cstr, length });
    }

    constexpr auto operator""_sid_hash(const char8_t* cstr, size_t length) noexcept -> ice::StringID_Hash
    {
        return stringid({ cstr, length }).hash_value;
    }

} // namespace ice
