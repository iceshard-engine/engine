/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/log_tag.hxx>
#include <ice/mem_allocator.hxx>
#include <ice/string/string.hxx>
#include <ice/string_utils.hxx>
#include <ice/param_list.hxx>

struct HSCPackerApp
{
    HSCPackerApp(ice::Allocator& alloc, ice::ParamList const& params);
    ~HSCPackerApp();

    ice::Allocator& _alloc;
};

bool hscp_param_validate_file(ice::ParamList const&, ice::ParamInfo const&, ice::String value) noexcept;

static constexpr ice::LogTagDefinition LogTag_Main = ice::create_log_tag(ice::LogTag::None, "hsc-packer");
static constexpr ice::LogTagDefinition LogTag_Details = ice::create_log_tag(LogTag_Main, "details");

static constexpr ice::ParamDefinition<ice::String> Param_Input{
    .name = "input",
    .name_short = "i",
    .description = "Defines the input for the asset compiler",
    .flags = ice::ParamFlags::IsRequired,
};

static constexpr ice::ParamDefinition<ice::String> Param_Output{
    .name = "out",
    .name_short = "o",
    .description = "Defines the output for the asset compiler",
    .flags = ice::ParamFlags::IsRequired,
};

static constexpr ice::ParamDefinition<ice::String> Param_Config{
    .name = "config",
    .name_short = "c",
    .description = "Defines the config for the asset compiler",
    .validator = hscp_param_validate_file,
    .flags = ice::ParamFlags::None,
};

static constexpr ice::ParamDefinition<bool> Param_Verbose{
    .name = "verbose",
    .name_short = "v",
    .description = "Enables detailed output during the packing process.",
    .flags = ice::ParamFlags::IsFlag,
};
