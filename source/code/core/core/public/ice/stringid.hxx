#pragma once
#include <ice/base.hxx>

namespace ice
{

    //! \brief Hashed representation of a \ref ice::String value.
    template<bool DebugFields>
    struct BaseStringID;

    //! \copy ice::BaseStringID.
    using StringID = BaseStringID<
        ice::config::StringID_DebugInfoEnabled
    >;

    //! \brief Argument type used to pass \ref ice::StringID values to functions.
    //!
    //! \note This allows the engine to control how \ref ice::StringID values are passed in the engine.
    //!     When \ref ice::config::StringID_DebugInfoEnabled is \b false values are passed by values.
    using StringID_Arg = std::conditional_t<
        ice::config::StringID_DebugInfoEnabled,
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


        using StringID_DebugNameHint = ice::utf8[24];

        struct StringID_DebugNameValue
        {
            ice::utf8 const* value;
            ice::utf8 consteval_flag;
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


    template<>
    struct BaseStringID<false>
    {
        using TypeTag = ice::TaggedStrongValue<ice::detail::stringid_type_v3::StringID_Tag>;

        detail::stringid_type_v3::StringID_Hash value;
    };

    template<>
    struct BaseStringID<true>
    {
        using TypeTag = ice::TaggedStrongValue<ice::detail::stringid_type_v3::StringID_Tag>;

        detail::stringid_type_v3::StringID_Hash value;
        detail::stringid_type_v3::StringID_DebugInfo debug_info;
    };


    constexpr auto stringid(std::u8string_view value) noexcept
    {
        using namespace ice::detail::murmur2_hash;
        mm2_x64_64 const hash_result = cexpr_murmur2_x64_64(value, ice::config::StringID_DefaultSeed);

        if constexpr (ice::config::StringID_DebugInfoEnabled)
        {
            if (std::is_constant_evaluated())
            {
                return BaseStringID<true> {
                    .value = { .value = hash_result.h[0] },
                        .debug_info = { .name_value = { value.data(), ice::utf8('\xff')} }
                };
            }
            else
            {
                BaseStringID<true> result{
                    .value = {.value = hash_result.h[0] },
                };

                size_t const cstr_size = value.size();
                size_t const origin_size = std::size(result.debug_info.name_hint);

                size_t const copy_count = std::min(origin_size, cstr_size);
                size_t const copy_offset = std::max(0ull, cstr_size - copy_count);


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

    constexpr auto stringid_hint(BaseStringID<false> val) noexcept -> std::u8string_view
    {
        return { };
    }

    constexpr auto stringid_hint(BaseStringID<true> const& val) noexcept -> std::u8string_view
    {
        if (std::is_constant_evaluated())
        {
            return val.debug_info.name_value.value;
        }
        else
        {
            if (val.debug_info.name_value.consteval_flag == ice::utf8('\xff'))
            {
                return val.debug_info.name_value.value;
            }
            else
            {
                return val.debug_info.name_hint;
            }
        }
    }

    template<bool HasDebugInfo>
    constexpr auto stringid_hash(BaseStringID<HasDebugInfo> val) noexcept -> detail::stringid_type_v3::StringID_Hash
    {
        return val.value;
    }

} // namespace ice
