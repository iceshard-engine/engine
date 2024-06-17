/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "hsc_reader_app.hxx"
#include <ice/os/windows.hxx>
#include <ice/module_register.hxx>
#include <ice/log_module.hxx>
#include <tuple>

bool ParamRange::param_parse_results(ParamRange& range, ice::Span<ice::String const> results) noexcept
{
    range.set = true;

    ice::String value = ice::span::front(results);
    ice::from_chars(value, value, range.start);

    // Assign the next value if we have multiple defined.
    if (ice::span::count(results) > 1)
    {
        value = results[1];
    }

    if (ice::string::any(value))
    {
        ice::from_chars(value, value, range.count);
    }
    return true;
}

void hscr_initialize_logging() noexcept
{
    static const std::tuple<ice::LogTagDefinition const&, bool*> tags[]{
        { LogTag_Main, &Param_HideHeader.value },
        { LogTag_InfoChunks, &Param_ShowChunks.value.set },
        { LogTag_InfoResources, &Param_ShowResources.value.set },
        { LogTag_InfoPaths, &Param_ShowResourcePaths.value },
    };

    for (auto const& tag : tags)
    {
        ice::log_tag_register(std::get<0>(tag));

        // Check if the tag can be enabled.
        bool* temp_param = std::get<1>(tag);
        ice::log_tag_enable(std::get<0>(tag).tag, *temp_param);
    }
}
