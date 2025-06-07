/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/log_tag.hxx>
#include <ice/mem_allocator.hxx>
#include <ice/string/string.hxx>
#include <ice/string_utils.hxx>
#include <ice/params.hxx>

void hscr_initialize_logging() noexcept;

static constexpr ice::LogTagDefinition LogTag_Main = ice::create_log_tag(ice::LogTag::None, "hsc-reader");
static constexpr ice::LogTagDefinition LogTag_InfoHeader = ice::create_log_tag(LogTag_Main, "header");
static constexpr ice::LogTagDefinition LogTag_InfoChunks = ice::create_log_tag(LogTag_Main, "chunks");
static constexpr ice::LogTagDefinition LogTag_InfoResources = ice::create_log_tag(LogTag_Main, "resource");
static constexpr ice::LogTagDefinition LogTag_InfoPaths = ice::create_log_tag(LogTag_Main, "paths");

struct ParamRange
{
    bool set = false;
    ice::u32 start = 0;
    ice::u32 count = ice::u32_max;

    static bool param_parse_results(ParamRange& out, ice::Span<ice::String const> results) noexcept;
};

static ice::ParamInstanceCustom<ParamRange> Param_ShowChunks{
    ice::ParamDefinition{
        .name = "-c,--chunks",
        .description = "Shows chunks information.",
        .type_name = "IDX:INT [COUNT:INT]",
        .min = 1, .max = 2,
        .flags = ice::ParamFlags::AllowExtraArgs
    }
};
static ice::ParamInstanceCustom<ParamRange> Param_ShowResources{
    ice::ParamDefinition{
        .name = "-r,--resources",
        .description = "Shows resources information.",
        .type_name = "IDX:INT [COUNT:INT]",
        .min = 1, .max = 2,
        .flags = ice::ParamFlags::AllowExtraArgs
    }
};
static ice::ParamInstance<bool> Param_ShowResourcePaths{
    "", "-p,--paths", "Shows resource path information."
};
static ice::ParamInstance<bool> Param_ShowCustomValues{
    "", "--custom-vals", "Shows app custom values (if the format supports them)."
};
static ice::ParamInstance<bool> Param_HideHeader{
    "", "--hide", "Hides header information.",
};
