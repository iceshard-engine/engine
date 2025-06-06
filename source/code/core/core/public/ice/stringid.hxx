/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>

namespace ice
{

    //! \brief Hashed representation of a \ref ice::String value.
    template<bool DebugFields>
    struct BaseStringID;

    //! \copy ice::BaseStringID.
    using StringID = BaseStringID<
        ice::build::Constant_StringID_DebugInfoEnabled
    >;

    //! \brief Argument type used to pass \ref ice::StringID values to functions.
    //!
    //! \note This allows the engine to control how \ref ice::StringID values are passed in the engine.
    //!     When \ref ice::build::Constant_StringID_DebugInfoEnabled is \b false values are passed by values.
    using StringID_Arg = std::conditional_t<
        ice::build::Constant_StringID_DebugInfoEnabled,
        StringID const&,
        StringID
    >;

    namespace detail::stringid_type_v3
    {

        //! \brief Internal hash type representing the hashed string value.
        struct StringID_Hash
        {
            using TypeTag = ice::StrongValue;

            ice::u64 value;
        };


        using StringID_DebugNameHint = char[24];

        struct StringID_DebugNameValue
        {
            char const* value;
            char consteval_flag;
        };

        union StringID_DebugInfo
        {
            StringID_DebugNameHint name_hint;
            StringID_DebugNameValue name_value;
        };

        //! \brief Tag type used to define \ref ice::BaseStringID as a \ref ice::TaggedStrongValue.
        //!
        //! \note This allows us to use common operators for \ref ice::BaseStringID<false> and \ref ice::BaseStringID<true> interchangeably.
        struct StringID_Tag;

    } // namespace detail::stringid_type_v3

    using detail::stringid_type_v3::StringID_Hash;


    template<>
    struct BaseStringID<false>
    {
        using TypeTag = ice::TaggedStrongValue<ice::detail::stringid_type_v3::StringID_Tag>;

        detail::stringid_type_v3::StringID_Hash value;

        constexpr operator ice::StringID_Hash() const noexcept;
        constexpr operator ice::BaseStringID<true>() const noexcept;
        constexpr bool operator==(ice::StringID_Hash strid_hash) const noexcept;
    };

    template<>
    struct BaseStringID<true>
    {
        using TypeTag = ice::TaggedStrongValue<ice::detail::stringid_type_v3::StringID_Tag>;

        detail::stringid_type_v3::StringID_Hash value;
        detail::stringid_type_v3::StringID_DebugInfo debug_info;

        constexpr operator ice::StringID_Hash() const noexcept;
        constexpr operator ice::BaseStringID<false>() const noexcept;
        constexpr bool operator==(ice::StringID_Hash strid_hash) const noexcept;
    };

    static constexpr ice::StringID StringID_Invalid{ .value = StringID_Hash{ } };

    constexpr auto stringid(std::string_view value) noexcept
    {
        using namespace ice::detail::murmur2_hash;
        mm2_x64_64 const hash_result = cexpr_murmur2_x64_64(value, ice::build::Constant_StringID_DefaultSeed);

        if constexpr (ice::build::Constant_StringID_DebugInfoEnabled)
        {
            if (std::is_constant_evaluated())
            {
                return BaseStringID<true> {
                    .value = { .value = hash_result.h[0] },
                    .debug_info = { .name_value = { value.data(), char('\xff') } }
                };
            }
            else
            {
                BaseStringID<true> result{
                    .value = { .value = hash_result.h[0] },
                    .debug_info = {.name_value = {.consteval_flag = '\0'}}
                };

                size_t const cstr_size = value.size();
                size_t const origin_size = std::size(result.debug_info.name_hint);

                size_t const copy_count = std::min(origin_size, cstr_size);
                size_t const copy_offset = std::max(size_t{ 0 }, cstr_size - copy_count);


                ice::i32 i = 0;
                for (auto& v : result.debug_info.name_hint)
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
                    result.debug_info.name_hint[0] = '~';
                }
                return result;
            }
        }
        else
        {
            return BaseStringID<false> {
                .value = { .value = hash_result.h[0] }
            };
        }
    }

    constexpr auto stringid(const char* string, size_t size) noexcept
    {
        return stringid(std::string_view{ string, size });
    }

    constexpr auto stringid_hint(ice::BaseStringID<false> val) noexcept -> std::string_view
    {
        return { };
    }

    constexpr auto stringid_hint(ice::BaseStringID<true> const& val) noexcept -> std::string_view
    {
        if (std::is_constant_evaluated())
        {
            return val.debug_info.name_value.value;
        }
        else
        {
            if (val.debug_info.name_value.consteval_flag == char('\xff'))
            {
                return val.debug_info.name_value.value;
            }
            else
            {
                char const* end = val.debug_info.name_hint + 23;
                // Need to find the end of the string (capped at 24 characters)
                while (*end == '\0' && end > val.debug_info.name_hint) --end;
                return { val.debug_info.name_hint, end + 1 }; // We add +1 here because we can't really test end[24]
            }
        }
    }

    template<bool HasDebugInfo>
    constexpr auto stringid_hash(ice::BaseStringID<HasDebugInfo> val) noexcept -> ice::detail::stringid_type_v3::StringID_Hash
    {
        return val.value;
    }

    constexpr auto hash(ice::StringID_Hash sid_hash) noexcept -> ice::u64
    {
        return sid_hash.value;
    }

    constexpr auto hash(ice::StringID_Arg value) noexcept -> ice::u64
    {
        return hash(stringid_hash(value));
    }


    constexpr auto operator""_sid(char const* str, size_t len) noexcept
    {
        return ice::stringid(str, len);
    }

    constexpr auto operator""_sid_hash(char const* str, size_t len) noexcept
    {
        return ice::stringid_hash(ice::stringid(str, len));
    }

    constexpr BaseStringID<false>::operator ice::StringID_Hash() const noexcept
    {
        return value;
    }

    constexpr BaseStringID<false>::operator ice::BaseStringID<true>() const noexcept
    {
        return { .value = value };
    }

    constexpr bool BaseStringID<false>::operator==(ice::StringID_Hash strid_hash) const noexcept
    {
        return value == strid_hash;
    }

    constexpr BaseStringID<true>::operator ice::StringID_Hash() const noexcept
    {
        return value;
    }

    constexpr BaseStringID<true>::operator ice::BaseStringID<false>() const noexcept
    {
        return { .value = value };
    }

    constexpr bool BaseStringID<true>::operator==(ice::StringID_Hash strid_hash) const noexcept
    {
        return value == strid_hash;
    }

    constexpr auto operator<=>(ice::StringID_Arg left, ice::StringID_Arg right) noexcept
    {
        return left.value.value <=> right.value.value;
    }

} // namespace ice
