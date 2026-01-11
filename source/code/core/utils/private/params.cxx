/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/params.hxx>
#include <ice/array.hxx>
#include <ice/string.hxx>
#include <ice/heap_string.hxx>
#include <ice/string_utils.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/native_file.hxx>

#include <CLI/CLI.hpp>
#include <variant>
#include <complex>

namespace ice
{

    ////////////////////////////////////////////////////////////////

    auto to_std(ice::String str) noexcept
    {
        return std::string{ str.begin(), str.end() };
    }

    auto to_std_or(ice::String str, std::string_view fallback) noexcept
    {
        if (str.not_empty())
        {
            return std::string{ str.begin(), str.end() };
        }
        else
        {
            return std::string{ fallback };
        }
    }

    static ice::ParamInstanceBase* Global_InstanceBase = nullptr;

    void params_register_globals(ice::Params& params) noexcept
    {
        ice::ParamInstanceBase* param_instances = Global_InstanceBase;
        while(param_instances != nullptr)
        {
            param_instances->on_register(params);
            param_instances = param_instances->_next;
        }
    }

    ParamInstanceBase::ParamInstanceBase(ice::String category, ice::String name, ice::String description) noexcept
        : ParamInstanceBase{ ParamDefinition{ .name = name, .description = description, .group = category } }
    {
    }

    ParamInstanceBase::ParamInstanceBase(ice::ParamDefinition const& definition) noexcept
        : _next{ std::exchange(Global_InstanceBase, this) }
        , definition{ definition }
    {
    }

    struct ParamsInternal
    {
        ice::Allocator& _allocator;
        CLI::App _app;

        ParamsInternal(
            ice::Allocator& alloc,
            ice::String name,
            ice::String description
        ) noexcept;
    };

    void delete_params(ParamsInternal* params) noexcept
    {
        params->_allocator.destroy(params);
    }

    auto create_params(
        ice::Allocator& alloc,
        ice::String name,
        ice::String version,
        ice::String description
    ) noexcept -> ice::Params
    {
        ice::HeapString<> description_with_version{ alloc, description };
        ice::string::push_format(description_with_version, " (v{})", version);
        return ice::make_unique<ice::ParamsInternal>(
            delete_params,
            alloc.create<ParamsInternal>(alloc, name, description_with_version)
        );
    }

    ParamsInternal::ParamsInternal(
        ice::Allocator& alloc,
        ice::String name,
        ice::String description
    ) noexcept
        : _allocator{ alloc }
        , _app{ to_std(description), to_std(name) }
    {
    }

    auto params_process_with_responsefile(
        ice::Params& params,
        ice::String response_file,
        std::string& out_cmdline
    ) noexcept -> ice::Result
    {
        ice::native_file::HeapFilePath filepath{ params->_allocator };
        ice::native_file::path_from_string(filepath, response_file);
        ice::native_file::File const file = ice::native_file::open_file(filepath, ice::native_file::FileOpenFlags::Read);
        if (file == false)
        {
            // Failed to open response file
            return E_Fail;
        }

        ice::usize const size_file = ice::native_file::sizeof_file(file);

        // Prepare the buffer where we store the file contents
        std::string cmdline{};
        cmdline.resize(size_file.value);

        ice::Memory const cmdline_mem{
            .location = cmdline.data(),
            .size = size_file,
            .alignment = ice::ualign::b_1
        };

        ice::usize const dataread = ice::native_file::read_file(file, size_file, cmdline_mem);
        // Failed to read the whole response file.
        if (dataread.value < cmdline.size())
        {
            return E_Fail;
        }

        out_cmdline = ice::move(cmdline);
        return S_Ok;
    }

    auto params_process(ice::Params& params, int argc, char const* const* argv) noexcept -> ice::i32
    {
        // Handle response files
        if (argc == 2 && argv[1][0] == '@')
        {
            std::string cmdline;
            if (params_process_with_responsefile(params, ice::String{ argv[1] + 1 }, cmdline) == E_Fail)
            {
                // TODO: Log the error once proper errors are defined
                return 1;
            }

            try
            {
                (params->_app).parse(std::move(cmdline), false);
            }
            catch (const CLI::ParseError& e)
            {
                return (params->_app).exit(e);
            };
        }
        else
        {
            try
            {
                (params->_app).parse(argc, argv);
            }
            catch (const CLI::ParseError& e)
            {
                return (params->_app).exit(e);
            };
        }
        return 0;
    }

    auto params_setup(CLI::Option* opt, ice::ParamDefinition const& definition, bool is_array_or_custom = false) noexcept
    {
        using PF = ParamFlags;
        opt->required(ice::has_any(definition.flags, PF::IsRequired));
        opt->allow_extra_args(ice::has_all(definition.flags, PF::AllowExtraArgs)); // Arrays have one additional check

        if (definition.description.not_empty())
        {
            opt->description(to_std(definition.description));
        }
        if (definition.group.not_empty())
        {
            opt->group(to_std(definition.group));
        }
        if (definition.type_name.not_empty())
        {
            opt->type_name(to_std(definition.type_name));
        }

        // Multi option policy
        if (ice::has_all(definition.flags, PF::TakeFirst))
        {
            opt->multi_option_policy(CLI::MultiOptionPolicy::TakeFirst);
        }
        else if (ice::has_all(definition.flags, PF::TakeLast))
        {
            opt->multi_option_policy(CLI::MultiOptionPolicy::TakeLast);
        }
        else if (ice::has_all(definition.flags, PF::TakeAll))
        {
            opt->multi_option_policy(CLI::MultiOptionPolicy::TakeAll);
        }

        if (is_array_or_custom)
        {
            opt->allow_extra_args(ice::has_none(definition.flags, PF::NoExtraArgs));

            if (definition.typesize.x != 0 || definition.typesize.y != ice::i32_max)
            {
                opt->inject_separator();
                opt->type_size(definition.typesize.x, definition.typesize.y);
            }

            if (definition.min != 0 || definition.max != ice::i32_max)
            {
                opt->expected(definition.min, definition.max);
            }

            if (definition.type_name.is_empty())
            {
                opt->type_name("VALUE");
            }
        }

#if ISP_LINUX
        static auto f = [](std::string p) noexcept -> std::string
        {
            size_t const start_quote = p.find_first_of('"');
            size_t const end_quote = p.find_last_of('"');
            if (start_quote != std::string::npos && end_quote != std::string::npos && start_quote < end_quote)
            {
                return p.substr(start_quote + 1, (end_quote - start_quote) - 1);
            }
            return p;
        };
#endif

        // Built-In Validators
        if (ice::has_all(definition.flags, PF::ValidatePath))
        {
#if ISP_LINUX
            opt->transform(f);
#endif
            opt->check(CLI::ExistingPath);
        }
        else if (ice::has_all(definition.flags, PF::ValidateFile))
        {
#if ISP_LINUX
            opt->transform(f);
#endif
            opt->check(CLI::ExistingFile);
        }
        else if (ice::has_all(definition.flags, PF::ValidateDirectory))
        {
#if ISP_LINUX
            opt->transform(f);
#endif
            opt->check(CLI::ExistingDirectory);
        }
        return opt;
    }

    template<typename T>
    bool params_define_internal(
        CLI::App& app,
        ice::Allocator&,
        ice::ParamDefinition const& definition,
        T& out_value
    ) noexcept
    {
        params_setup(app.add_option(to_std(definition.name), out_value), definition);
        return true;
    }

    template<>
    bool params_define_internal<bool>(
        CLI::App& app,
        ice::Allocator&,
        ice::ParamDefinition const& definition,
        bool& out_value
    ) noexcept
    {
        params_setup(app.add_flag(to_std(definition.name), out_value), definition);
        return true;
    }

    template<>
    bool params_define_internal<ice::String>(
        CLI::App& app,
        ice::Allocator&,
        ice::ParamDefinition const& definition,
        ice::String& out_value
    ) noexcept
    {
        auto fn_callback = [&out_value](CLI::results_t const& results) noexcept
        {
            out_value = ice::String{ results.front() };
            return true;
        };
        params_setup(app.add_option(to_std(definition.name), ice::move(fn_callback)), definition);
        return true;
    }

    template<>
    bool params_define_internal<ice::HeapString<>>(
        CLI::App& app,
        ice::Allocator&,
        ice::ParamDefinition const& definition,
        ice::HeapString<>& out_value
    ) noexcept
    {
        auto fn_callback = [&out_value](CLI::results_t const& results) noexcept
        {
            std::string const& arg = results.front();
            out_value.push_back(ice::String{ std::string_view{ arg } });
            return true;
        };
        params_setup(app.add_option(to_std(definition.name), ice::move(fn_callback)), definition);
        return true;
    }

    template<>
    bool params_define_internal<ice::Array<ice::String>>(
        CLI::App& app,
        ice::Allocator&,
        ice::ParamDefinition const& definition,
        ice::Array<ice::String>& out_values
    ) noexcept
    {
        auto fn_callback = [&out_values](CLI::results_t const& results) noexcept
        {
            for (std::string const& result : results)
            {
                out_values.push_back({ result.data(), static_cast<ice::u32>(result.size()) });
            }
            return true;
        };
        params_setup(app.add_option(to_std(definition.name), ice::move(fn_callback)), definition, true);
        return true;
    }

    template<>
    bool params_define_internal<ice::Array<ice::HeapString<>>>(
        CLI::App& app,
        ice::Allocator& alloc,
        ice::ParamDefinition const& definition,
        ice::Array<ice::HeapString<>>& out_values
    ) noexcept
    {
        auto fn_callback = [&alloc, &out_values](CLI::results_t const& results) noexcept
        {
            for (std::string const& result : results)
            {
                out_values.push_back({ alloc, ice::String{ std::string_view{ result } } });
            }
            return true;
        };
        params_setup(app.add_option(to_std(definition.name), ice::move(fn_callback)), definition);
        return true;
    }

    bool params_define_custom(
        ice::Params& params,
        ice::ParamDefinition const& definition,
        void* ice_userdata,
        ice::ParamsCustomCallback ice_callback
    ) noexcept
    {
        auto fn_callback = [
            ice_userdata,
            ice_callback,
            typesize = definition.typesize,
            &alloc = params->_allocator
        ](CLI::results_t const& results) noexcept
        {
            ice::u32 const result_count = (ice::u32) std::min<ice::usize::base_type>(
                    results.size(), std::max(typesize.y, 0)
            );
            ice::StackAllocator<ice::size_of<ice::String> * 8> stack_alloc;
            ice::Array<ice::String> ice_results{ result_count <= 8 ? stack_alloc : alloc };
            ice_results.reserve(result_count);

            auto it = results.begin();
            auto const end = results.end();

            bool valid = true;
            while (it != end && valid)
            {
                std::string_view const result{ *it };
                if (ice_results.size() == result_count || result.empty())
                {
                    valid &= ice_callback(ice_userdata, ice_results);
                    ice_results.clear();
                }
                else if (result.empty() == false)
                {
                    ice_results.push_back(result);
                }
                it += 1;
            }

            if (valid)
            {
                valid &= ice_callback(ice_userdata, ice_results);
            }

            return valid;
        };
        params_setup(params->_app.add_option(to_std(definition.name), ice::move(fn_callback)), definition, true);
        return true;
    }

#define IMPL_PARAMS_DEFINE(type) \
    template<> bool params_define<type>(ice::Params& params, ice::ParamDefinition const& definition, type& out_value) noexcept \
    { \
        CLI::App& app = params->_app; \
        return params_define_internal(app, params->_allocator, definition, out_value); \
    }

    IMPL_PARAMS_DEFINE(bool)
    IMPL_PARAMS_DEFINE(char)
    // IMPL_PARAMS_DEFINE(utf16)
    // IMPL_PARAMS_DEFINE(utf32)
    // IMPL_PARAMS_DEFINE(wchar)
    IMPL_PARAMS_DEFINE(f32)
    IMPL_PARAMS_DEFINE(f64)
    IMPL_PARAMS_DEFINE(i8)
    IMPL_PARAMS_DEFINE(i16)
    IMPL_PARAMS_DEFINE(i32)
    IMPL_PARAMS_DEFINE(i64)
    IMPL_PARAMS_DEFINE(u8)
    IMPL_PARAMS_DEFINE(u16)
    IMPL_PARAMS_DEFINE(u32)
    IMPL_PARAMS_DEFINE(u64)
    IMPL_PARAMS_DEFINE(ice::String)
    IMPL_PARAMS_DEFINE(ice::HeapString<>)
    IMPL_PARAMS_DEFINE(ice::Array<ice::String>)
    IMPL_PARAMS_DEFINE(ice::Array<ice::HeapString<>>)

#undef IMPL_PARAMS_DEFINE

} // namespace ice
