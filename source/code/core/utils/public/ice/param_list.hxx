#pragma once
#include <ice/container/hashmap.hxx>
#include <ice/string/string.hxx>
#include <ice/string_utils.hxx>
#include <ice/span.hxx>

namespace ice
{

    //! \brief A list of pre-parsed, but unresolved, parameters.
    struct ParamList;

    //! \brief Basic param information representing parts of a param definition.
    struct ParamInfo;

    //! \brief Parameter definition used to find values in the param list.
    template<typename T>
    struct ParamDefinition;

    //! \brief Flags used change parameter 'find' behavior.
    enum class ParamFlags : ice::u8;

    namespace params
    {

        //! \brief Clears all definitions and arguments form the given parameter list.
        inline void clear(ice::ParamList& list) noexcept;

        //! \brief Clears all pushed arguments from the given parameter list.
        //! \note All defined params are still kept.
        inline void clear_args(ice::ParamList& list) noexcept;

        //! \brief Define a parameter that could be used to construct 'help' output.
        //! \todo The definitions are currently unused.
        inline void define(ice::ParamList& list, ice::ParamInfo const& definition) noexcept;

        //! \brief Pushes a new argument into the parameter list, the argument may be pushed with a value.
        //! \note This function does not validate pushed values.
        inline bool push_arg(ice::ParamList& list, ice::String arg, ice::String value = "") noexcept;

        //! \brief Pushes all arguments into the parameter list.
        //! \note This function does not validate pushed values.
        inline bool push_args(ice::ParamList& list, ice::Span<char const* const> args) noexcept;

        //! \brief Return all defined parameters.
        inline auto definitions(ice::ParamList const& list) noexcept -> ice::Span<ice::ParamInfo const>;

        //! \return 'true' If at least one parameter is found.
        template<typename Value>
        inline bool has_any(
            ice::ParamList const& list,
            ice::ParamDefinition<Value> const& def
        ) noexcept;

        //! \brief Find the FIRST occurence of a parameter and return its value if present.
        //! \return 'true' If the parameter was found.
        template<typename Value>
        inline bool find_first(
            ice::ParamList const& list,
            ice::ParamDefinition<Value> const& def,
            Value& out_value
        ) noexcept;

        //! \brief Find the LAST occurence of a parameter and return its value if present.
        //! \return 'true' If the parameter was found.
        template<typename Value>
        inline bool find_last(
            ice::ParamList const& list,
            ice::ParamDefinition<Value> const& def,
            Value& out_value
        ) noexcept;

        //! \brief Find ALL occurences of a parameter and append values in the given array.
        //! \note This function collect parameters from both the 'long' and 'short' forms.
        //! \return 'true' If at least one parameter was found.
        template<typename Value>
        inline bool find_all(
            ice::ParamList const& list,
            ice::ParamDefinition<Value> const& def,
            ice::Array<Value>& out_values
        ) noexcept;

    } // namespace params

    using ParamValidateFn = bool(*)(
        ice::ParamList const& params,
        ice::ParamInfo const& info,
        ice::String value
    ) noexcept;

    template<typename V>
    using ParamParseFn = auto(*)(
        ice::ParamList const& params,
        ice::ParamInfo const& info,
        ice::String raw_value,
        V& out_value
    ) noexcept -> ice::ucount;

    enum class ParamFlags : ice::u8
    {
        None = 0x00,
        IsFlag = 0x01,
        IsRequired = 0x02,

        All = IsFlag | IsRequired,
    };

    struct ParamInfo
    {
        ice::String name;
        ice::String name_short;
        ice::String description;
    };

    template<typename T>
    struct ParamDefinition
    {
        ice::String name;
        ice::String name_short = "";
        ice::String description = "";

        ice::ParamValidateFn validator = nullptr;
        ice::ParamParseFn<T> parser = nullptr;
        ice::ParamFlags flags = ParamFlags::None;

        constexpr operator ParamInfo() const noexcept;
    };

    template<typename T>
    constexpr ParamDefinition<T>::operator ParamInfo() const noexcept
    {
        return {
            .name = name,
            .name_short = name_short,
            .description = description
        };
    }

    struct ParamList
    {
        struct ParamInfo
        {
            ice::u8 first_index;
            ice::u8 count;
            bool is_short : 1;
            bool can_have_value : 1;
        };

        ice::HashMap<ParamInfo> _params;
        ice::Array<ice::ParamInfo> _definitions;
        ice::Array<ice::String> _values;

        inline ParamList(ice::Allocator& alloc) noexcept;
        inline ParamList(ice::Allocator& alloc, int argc, char const* const* argv) noexcept;
        inline ParamList(ice::Allocator& alloc, ice::Span<char const* const> args) noexcept;
        ~ParamList() noexcept = default;
    };

    inline ParamList::ParamList(ice::Allocator& alloc) noexcept
        : _definitions{ alloc }
        , _values{ alloc }
        , _params{ alloc }
    {
    }

    inline ParamList::ParamList(ice::Allocator& alloc, int argc, char const* const* argv) noexcept
        : ParamList{ alloc, { argv, ice::ucount(argc) } }
    {
    }

    inline ParamList::ParamList(ice::Allocator& alloc, ice::Span<char const* const> args) noexcept
        : _definitions{ alloc }
        , _values{ alloc }
        , _params{ alloc }
    {
        ice::params::push_args(*this, args);
    }

    namespace detail
    {

        using ParamInfo = ParamList::ParamInfo;

        inline bool default_param_validator(
            ice::ParamList const& /*params*/,
            ice::ParamInfo const& /*def*/,
            ice::String value
        ) noexcept
        {
            return ice::string::any(value);
        }

        template<typename T>
        inline auto default_param_parser(
            ice::ParamList const& /*params*/,
            ice::ParamInfo const& /*def*/,
            ice::String raw_value,
            T& out_value
        ) noexcept -> ice::ucount
        {
            if constexpr (std::is_same_v<T, ice::String>)
            {
                out_value = raw_value;
                return 1;
            }
            else if constexpr (std::is_integral_v<T> && std::is_same_v<T, bool> == false)
            {
                T value;
                ice::FromCharsResult<ice::String> result = ice::from_chars(raw_value, value);
                if (result)
                {
                    out_value = value;
                    return 1;
                }
            }
            else
            {
                return 0;
            }
        }

        inline bool parse_option(ice::String& value, bool& is_short) noexcept
        {
            ice::u32 const option_offset = ice::string::find_first_not_of(value, '-');
            if (option_offset > 0)
            {
                is_short = option_offset == 1;
                value = ice::string::substr(value, option_offset);
            }
            return option_offset > 0;
        }

        inline bool parse_option(ice::String& value) noexcept
        {
            bool unused;
            return parse_option(value, unused);
        }

        template<typename Fn>
        inline void for_each_option(ice::String value, Fn&& fn) noexcept
        {
            bool is_short = false;
            if (detail::parse_option(value, is_short))
            {
                if (is_short)
                {
                    ice::ucount const option_count = ice::string::size(value);
                    for (ice::u32 idx = 0; idx < option_count; ++idx)
                    {
                        ice::forward<Fn>(fn)(
                            ice::string::substr(value, idx, 1),
                            /* can_have_value */ (idx + 1) == option_count
                        );
                    }
                }
                else
                {
                    ice::forward<Fn>(fn)(value, /* can_have_value */true);
                }
            }
        }

        template<typename T>
        inline auto get_paraminfo(
            ice::HashMap<ParamInfo> const& params,
            ice::ParamDefinition<T> const& def
        ) noexcept -> ice::detail::ParamInfo
        {
            detail::ParamInfo combined_info{ .first_index = ice::u8_max };
            for (ice::String key : { def.name_short, def.name })
            {
                if (ice::string::empty(key))
                {
                    continue;
                }

                detail::ParamInfo const* paraminfo = ice::hashmap::try_get(
                    params,
                    ice::hash(key)
                );
                if (paraminfo != nullptr)
                {
                    combined_info.can_have_value |= paraminfo->can_have_value;
                    combined_info.first_index = ice::min(paraminfo->first_index, combined_info.first_index);
                    combined_info.count += paraminfo->count;
                }
            }
            return combined_info;
        }

        template<typename T>
        inline bool read_param(
            ice::ParamList const& params,
            ice::detail::ParamInfo info,
            ice::ParamDefinition<T> const& def,
            T& out_value,
            bool find_last
        ) noexcept
        {
            if (ice::has_all(def.flags, ParamFlags::IsFlag))
            {
                ICE_ASSERT_CORE(std::is_integral_v<T>);
                if constexpr (std::is_integral_v<T>)
                {
                    out_value = static_cast<T>(true);
                }
                return true;
            }
            // We are actually looking for the next value if it's not a flag.
            else if ((info.first_index + 1u) == ice::count(params._values))
            {
                return false;
            }

            info.count -= 1;
            ice::u32 idx = info.first_index + 1;
            while (find_last && info.count > 0)
            {
                bool is_short = false;
                ice::String option = params._values[idx];
                if (detail::parse_option(option, is_short))
                {
                    info.count -= is_short ? option == def.name_short : option == def.name;
                }
                idx += 1;
            }

            // We don't have a value required
            ice::String raw_value = params._values[idx];
            if (raw_value[0] == '-')
            {
                return false;
            }

            // Validate the value
            bool is_valid = def.validator == nullptr || def.validator(params, def, raw_value);
            if (is_valid)
            {
                ice::ParamParseFn<T> fn_parser = def.parser == nullptr
                    ? default_param_parser<T>
                    : def.parser;

                is_valid = fn_parser(params, def, raw_value, out_value) > 0;
            }
            return is_valid;
        }

        template<typename T>
        inline bool read_param(
            ice::ParamList const& params,
            ice::detail::ParamInfo info,
            ice::ParamDefinition<T> const& def,
            ice::Array<T>& out_values
        ) noexcept
        {
            if ((info.first_index + 1u) == ice::count(params._values))
            {
                return false;
            }

            // We don't have a value required
            bool next_value = true;
            ice::u32 added = 0;
            ice::u32 count = info.count;
            ice::u32 idx = info.first_index;
            while (count > 0 || next_value)
            {
                bool is_short;
                ice::String option = params._values[idx];
                if (detail::parse_option(option, is_short))
                {
                    next_value = is_short ? option == def.name_short : option == def.name;
                    count -= ice::u32(next_value);
                }
                else if (next_value)
                {
                    next_value = false;

                    // Remove the quotes
                    ice::String const raw_value = params._values[idx];
                    if (def.validator == nullptr || def.validator(params, def, raw_value))
                    {
                        ice::ParamParseFn<T> fn_parser = def.parser == nullptr
                            ? default_param_parser<T>
                            : def.parser;

                        T temp_value{};
                        ice::ucount const consumed = fn_parser(params, def, raw_value, temp_value);
                        if (consumed == 0)
                        {
                            break;
                        }

                        ice::array::push_back(out_values, ice::move(temp_value));
                        idx += consumed - 1;
                        added += 1;
                    }
                }
                idx += 1;
            }

            // Validate the value
            return added == info.count;
        }

    } // namespace detail

    namespace params
    {

        inline void clear(ice::ParamList& list) noexcept
        {
            ice::array::clear(list._definitions);
            ice::params::clear_args(list);
        }

        inline void clear_args(ice::ParamList& list) noexcept
        {
            ice::array::clear(list._values);
            ice::hashmap::clear(list._params);
        }

        inline void define(ice::ParamList& list, ice::ParamInfo const& definition) noexcept
        {
            ice::array::push_back(list._definitions, definition);
        }

        inline bool push_arg(ice::ParamList& list, ice::String arg, ice::String value) noexcept
        {
            ice::u8 argidx = ice::u8(ice::array::count(list._values));
            ice::array::push_back(list._values, arg);
            detail::for_each_option(
                ice::array::back(list._values),
                [&list, argidx](ice::String argname, bool can_have_value) noexcept
                {
                    detail::ParamInfo& info = ice::hashmap::get_or_set(
                        list._params,
                        ice::hash(argname),
                        detail::ParamInfo{
                            .first_index = argidx,
                            .count = 0,
                            .is_short = ice::string::size(argname) == 1,
                            .can_have_value = can_have_value
                        }
                    );
                    info.count += 1;
                }
            );
            if (ice::string::any(value))
            {
                ice::array::push_back(list._values, value);
            }
            return true;
        }

        inline bool push_args(ice::ParamList& list, ice::Span<char const* const> args) noexcept
        {
            ice::array::grow(
                list._values,
                ice::array::count(list._values) + ice::count(args)
            );

            for (char const* argv : args)
            {
                ice::params::push_arg(list, argv);
            }
            return true;
        }

        inline auto definitions(ice::ParamList const& list) noexcept -> ice::Span<ice::ParamInfo const>
        {
            return list._definitions;
        }

        template<typename Value>
        inline bool has_any(
            ice::ParamList const& list,
            ice::ParamDefinition<Value> const& def
        ) noexcept
        {
            detail::ParamInfo const info = detail::get_paraminfo(list._params, def);
            if (info.first_index == ice::u8_max)
            {
                return false;
            }

            ice::String raw_value = list._values[info.first_index];
            if (detail::parse_option(raw_value))
            {
                ICE_ASSERT_CORE(raw_value == def.name || ice::string::find_first_of(raw_value, def.name_short) != ice::String_NPos);
                return true;
            }
            return false;
        }

        template<typename Value>
        inline bool find_first(
            ice::ParamList const& list,
            ice::ParamDefinition<Value> const& def,
            Value& out_value
        ) noexcept
        {
            detail::ParamInfo const info = detail::get_paraminfo(list._params, def);
            if (info.first_index == ice::u8_max)
            {
                return false;
            }

            ice::String raw_value = list._values[info.first_index];
            if (detail::parse_option(raw_value))
            {
                ICE_ASSERT_CORE(raw_value == def.name || ice::string::find_first_of(raw_value, def.name_short) != ice::String_NPos);
                return detail::read_param(list, info, def, out_value, /* find last */ false);
            }
            return false;
        }

        template<typename Value>
        inline bool find_last(
            ice::ParamList const& list,
            ice::ParamDefinition<Value> const& def,
            Value& out_value
        ) noexcept
        {
            detail::ParamInfo const info = detail::get_paraminfo(list._params, def);
            if (info.first_index == ice::u8_max)
            {
                return false;
            }

            ice::String raw_value = list._values[info.first_index];
            if (detail::parse_option(raw_value))
            {
                ICE_ASSERT_CORE(raw_value == def.name || ice::string::find_first_of(raw_value, def.name_short) != ice::String_NPos);
                return detail::read_param(list, info, def, out_value, /* find last */ true);
            }
            return false;
        }

        template<typename Value>
        inline bool find_all(
            ice::ParamList const& list,
            ice::ParamDefinition<Value> const& def,
            ice::Array<Value>& out_values
        ) noexcept
        {
            detail::ParamInfo const info = detail::get_paraminfo(list._params, def);
            if (info.first_index == ice::u8_max)
            {
                return false;
            }

            ice::String raw_value = list._values[info.first_index];
            if (detail::parse_option(raw_value))
            {
                ICE_ASSERT_CORE(raw_value == def.name || ice::string::find_first_of(raw_value, def.name_short) != ice::String_NPos);
                return detail::read_param(list, info, def, out_values);
            }
            return false;
        }

    } // namespace params

} // namespace ice
