#pragma once
#include <core/cexpr/hash.hxx>
#include <fmt/format.h>

#if CFG_RELEASE == 0
#    define STRINGID_DEBUG 1
#else
#    define STRINGID_DEBUG 0
#endif

namespace core::cexpr
{

    //! \brief The hash type of the stringid type.
    enum class stringid_hash_type : uint64_t
    {
    };

    //! \brief The stringid type which generally is a hash value from a string.
    struct stringid_type
    {
        //! \brief The hash value.
        stringid_hash_type hash_value;

#if STRINGID_DEBUG == 1
        //! \brief The origin string (or part of it).
        char hash_origin[24];
#endif
    };

    constexpr stringid_type stringid_invalid{ stringid_hash_type{ 0 } };

#if STRINGID_DEBUG == 1

    //! \brief Create a new stringid_type value from the given string.
    inline auto stringid(std::string_view cstr) noexcept -> stringid_type
    {
        stringid_type result{ static_cast<core::cexpr::stringid_hash_type>(core::cexpr::hash_cstring(cstr.data(), cstr.length())), "" };
        if (cstr.length() > 24 - 1)
        {
            auto offset = cstr.length() - 20;
            std::memset(result.hash_origin, '.', sizeof("..."));
            std::memcpy(result.hash_origin + 3, cstr.data() + offset, 20);
        }
        else
        {
            std::memcpy(result.hash_origin, cstr.data(), cstr.length());
        }
        return result;
    }

    //! \brief Creates a new constexpr stringid_type value from the given string.
    inline constexpr auto stringid_cexpr_(std::string_view cstr) noexcept -> stringid_type
    {
        stringid_type result{ static_cast<core::cexpr::stringid_hash_type>(core::cexpr::hash_cstring(cstr.data(), cstr.length())), "" };
        return result;
    }

    //! \brief The argument type used to pass stringid_type values.
    using stringid_argument_type = const stringid_type&;

#    define stringid_cexpr(name) \
        stringid_type { core::cexpr::stringid_cexpr_(name).hash_value, name }

#else

    //! \brief Create a new stringid_type value from the given string.
    constexpr auto stringid(std::string_view cstr) noexcept -> stringid_type
    {
        return { static_cast<core::cexpr::stringid_hash_type>(core::cexpr::hash_cstring(cstr.data(), cstr.length())) };
    }

    //! \brief Creates a new constexpr stringid_type value from the given string.
    constexpr auto stringid_cexpr(std::string_view cstr) noexcept -> stringid_type
    {
        return stringid(cstr);
    }

    //! \brief The argument type used to pass stringid_type values.
    using stringid_argument_type = stringid_type;

#endif

    //! \brief Equality operator for stringid_type values.
    inline bool operator==(stringid_argument_type left, stringid_argument_type right) noexcept
    {
        return left.hash_value == right.hash_value;
    }

    //! \brief Inequality operator for stringid_type values.
    inline bool operator!=(stringid_argument_type left, stringid_argument_type right) noexcept
    {
        return !(left == right);
    }

} // namespace core::cexpr

//////////////////////////////////////////////////////////////////////////

namespace fmt
{

    template<>
    struct formatter<core::cexpr::stringid_type>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext& ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const core::cexpr::stringid_type& strid, FormatContext& ctx)
        {
            if (strid.hash_value == core::cexpr::stringid_hash_type{ 0 })
            {
                return fmt::format_to(ctx.begin(), "[sid:<invalid>]");
            }
            else
            {
#if STRINGID_DEBUG == 1
                return fmt::format_to(ctx.begin(), "[sid:{:16x}]'{}'", static_cast<std::underlying_type_t<decltype(strid.hash_value)>>(strid.hash_value), strid.hash_origin);
#else
                return fmt::format_to(ctx.begin(), "[sid:{:16x}]", static_cast<std::underlying_type_t<decltype(strid.hash_value)>>(strid.hash_value));
#endif
            }
        }
    };

} // namespace fmt
