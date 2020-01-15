#pragma once
#include <core/cexpr/hash.hxx>
#include <fmt/format.h>
#include <string_view>

namespace core
{

    namespace cexpr
    {

        enum class stringid_hash_type : uint64_t
        {
            Invalid = 0x0
        };

        template<bool debug_fields = false>
        struct stringid_base_type
        {
            stringid_hash_type hash_value;
        };

        template<>
        struct stringid_base_type<true>
        {
            stringid_hash_type hash_value;

            char hash_origin[24];
        };

        template<bool debug_implementation = false>
        inline constexpr auto stringid(std::string_view cstr) noexcept -> stringid_base_type<debug_implementation>
        {
            return { core::cexpr::stringid_hash_type{ core::cexpr::hash_cstring(cstr.data(), cstr.length()) } };
        }

        template<>
        inline constexpr auto stringid<true>(std::string_view cstr) noexcept -> stringid_base_type<true>
        {
            stringid_base_type<true> result{ core::cexpr::stringid_hash_type{ core::cexpr::hash_cstring(cstr.data(), cstr.length()) } };

            int32_t const cstr_size = static_cast<int32_t>(cstr.size());
            int32_t const origin_size = static_cast<int32_t>(std::size(result.hash_origin));

            int32_t const copy_count = std::min(origin_size, cstr_size);
            int32_t const copy_offset = std::max(0, cstr_size - copy_count);

            int32_t i = 0;
            for (auto& v : result.hash_origin)
            {
                if (i < copy_count)
                {
                    v = cstr[copy_offset + i];
                }
                i++;
            }

            if (copy_offset > 0)
            {
                result.hash_origin[0] = result.hash_origin[1] = result.hash_origin[2] = '.';
            }

            return result;
        }

        template<bool debug_fields>
        struct stringid_defined_types
        {
            using stringid_type = stringid_base_type<debug_fields>;

            using stringid_arg_type = stringid_type const&;

            using stringid_hash_type = stringid_hash_type;
        };

        template<>
        struct stringid_defined_types<true>
        {
            using stringid_type = stringid_base_type<true>;

            using stringid_arg_type = stringid_type const&;

            using stringid_hash_type = stringid_hash_type;
        };

        template<typename T = stringid_base_type<false>>
        inline constexpr auto origin_value(T const&) noexcept -> std::string_view
        {
            return {};
        }

        template<>
        inline constexpr auto origin_value<stringid_base_type<true>>(stringid_base_type<true> const& value) noexcept -> std::string_view
        {
            return value.hash_origin;
        }

        template<typename T = stringid_base_type<false>>
        inline constexpr auto has_debug_fields() noexcept
        {
            return false;
        }

        template<>
        inline constexpr auto has_debug_fields<stringid_base_type<true>>() noexcept
        {
            return true;
        }

        inline constexpr auto operator==(
            typename stringid_defined_types<true>::stringid_arg_type left,
            typename stringid_defined_types<true>::stringid_arg_type right
        ) noexcept -> bool
        {
            return left.hash_value == right.hash_value;
        }

        inline constexpr auto operator!=(
            typename stringid_defined_types<true>::stringid_arg_type left,
            typename stringid_defined_types<true>::stringid_arg_type right
        ) noexcept -> bool
        {
            return !(left == right);
        }

        inline constexpr auto operator==(
            typename stringid_defined_types<false>::stringid_arg_type left,
            typename stringid_defined_types<false>::stringid_arg_type right
        ) noexcept -> bool
        {
            return left.hash_value == right.hash_value;
        }

        inline constexpr auto operator!=(
            typename stringid_defined_types<false>::stringid_arg_type left,
            typename stringid_defined_types<false>::stringid_arg_type right
        ) noexcept -> bool
        {
            return !(left == right);
        }

    } // namespace cexpr

    using stringid_type = core::cexpr::stringid_defined_types<core::build::is_release == false>::stringid_type;

    using stringid_arg_type = core::cexpr::stringid_defined_types<core::build::is_release == false>::stringid_arg_type;

    using stringid_hash_type = core::cexpr::stringid_defined_types<core::build::is_release == false>::stringid_hash_type;

    static constexpr stringid_type stringid_invalid{ stringid_hash_type{ 0 } };

    inline constexpr auto operator""_sid(const char* cstr, size_t length) noexcept -> core::stringid_type
    {
        return core::cexpr::stringid<core::build::is_release == false>({ cstr, length });
    }

    inline constexpr auto stringid(std::string_view value) noexcept -> core::stringid_type
    {
        return core::cexpr::stringid<core::build::is_release == false>(value);
    }

    inline constexpr auto origin(stringid_arg_type value) noexcept -> std::string_view
    {
        return core::cexpr::origin_value<stringid_type>(value);
    }

    inline constexpr auto hash(stringid_arg_type value) noexcept -> std::underlying_type_t<stringid_hash_type>
    {
        return static_cast<std::underlying_type_t<stringid_hash_type>>(value.hash_value);
    }

} // namespace core

using core::operator""_sid;

namespace fmt
{

    template<bool debug_fields>
    struct formatter<core::cexpr::stringid_base_type<debug_fields>>
    {
        using stringid_arg_type = typename core::cexpr::stringid_defined_types<debug_fields>::stringid_arg_type;

        template<typename ParseContext>
        constexpr auto parse(ParseContext& ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        constexpr auto format(stringid_arg_type value, FormatContext& ctx)
        {
            if (value == core::stringid_invalid)
            {
                return fmt::format_to(ctx.out(), "[sid:<invalid>]");
            }
            else
            {
                if constexpr (debug_fields == false)
                {
                    return fmt::format_to(ctx.out(), "[sid:{:16x}]", core::hash(value));
                }
                else
                {
                    return fmt::format_to(ctx.out(), "[sid:{:16x}]'{}'", core::hash(value), value.hash_origin);
                }
            }
        }
    };

} // namespace fmt
