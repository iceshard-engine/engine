#pragma once
#include <ice/log_tag.hxx>
#include <ice/mem_allocator.hxx>
#include <ice/string/string.hxx>
#include <ice/string_utils.hxx>
#include <ice/param_list.hxx>

struct HSCReaderApp
{
    HSCReaderApp(ice::Allocator& alloc, ice::ParamList const& params);
    ~HSCReaderApp();

    ice::Allocator& _alloc;
};

bool hscr_param_validate_file(ice::ParamList const&, ice::ParamInfo const&, ice::String value) noexcept;

static constexpr ice::LogTagDefinition LogTag_Main = ice::create_log_tag(ice::LogTag::None, "hsc-reader");
static constexpr ice::LogTagDefinition LogTag_InfoHeader = ice::create_log_tag(LogTag_Main, "header");
static constexpr ice::LogTagDefinition LogTag_InfoChunks = ice::create_log_tag(LogTag_Main, "chunks");
static constexpr ice::LogTagDefinition LogTag_InfoResources = ice::create_log_tag(LogTag_Main, "resource");
static constexpr ice::LogTagDefinition LogTag_InfoPaths = ice::create_log_tag(LogTag_Main, "paths");

struct ParamRange
{
    ice::u32 start = 0;
    ice::u32 count = ice::u32_max;

    static bool param_validate(
        ice::ParamList const& list,
        ice::ParamInfo const& info,
        ice::String value
    ) noexcept;

    static auto param_parse(
        ice::ParamList const& list,
        ice::ParamInfo const& info,
        ice::String value,
        ParamRange& out_range
    ) noexcept -> ice::u32;
};

static constexpr ice::ParamDefinition<ice::String> Param_File{
    .name = "file",
    .name_short = "f",
    .description = "The HailStorm pack file to be inspected.",
    .validator = hscr_param_validate_file,
    .flags = ice::ParamFlags::IsRequired,
};

static constexpr ice::ParamDefinition<bool> Param_InfoHeader{
    .name = "header",
    .name_short = "h",
    .description = "Shows header information.",
    .flags = ice::ParamFlags::IsFlag,
};

static constexpr ice::ParamDefinition<ParamRange> Param_InfoChunks{
    .name = "chunks",
    .name_short = "c",
    .description = "Shows chunk information.",
    .validator = ParamRange::param_validate,
    .parser = ParamRange::param_parse,
};

static constexpr ice::ParamDefinition<ParamRange> Param_InfoResources{
    .name = "resources",
    .name_short = "r",
    .description = "Shows resource information.",
    .validator = ParamRange::param_validate,
    .parser = ParamRange::param_parse,
};

static constexpr ice::ParamDefinition<bool> Param_InfoCustomValues{
    .name = "show-custom-values",
    .description = "Shows app custom values (if the format supports them).",
    .flags = ice::ParamFlags::IsFlag,
};

static constexpr ice::ParamDefinition<bool> Param_InfoResourcePaths{
    .name = "paths",
    .name_short = "p",
    .description = "Shows resource path information.",
    .flags = ice::ParamFlags::IsFlag,
};

bool ParamRange::param_validate(
    ice::ParamList const& list,
    ice::ParamInfo const& info,
    ice::String value
) noexcept
{
    return ice::string::find_first_not_of(value, ice::String{ "0123456789," }) == ice::String_NPos;
}

auto ParamRange::param_parse(
    ice::ParamList const& list,
    ice::ParamInfo const& info,
    ice::String value,
    ParamRange& out_range
) noexcept -> ice::u32
{
    ice::from_chars(value, value, out_range.start);
    if (ice::string::any(value))
    {
        ice::from_chars(ice::string::substr(value, 1), out_range.count);
    }
    return 1;
}
