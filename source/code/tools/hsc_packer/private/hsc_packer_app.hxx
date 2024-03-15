/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/log.hxx>
#include <ice/log_tag.hxx>
#include <ice/mem_allocator.hxx>
#include <ice/string/string.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/string_utils.hxx>
#include <ice/param_list.hxx>
#include <ice/native_file.hxx>

bool hscp_param_validate_file(ice::ParamList const&, ice::ParamInfo const&, ice::String value) noexcept;
auto hscp_process_directory(ice::Allocator& alloc, ice::String dir) noexcept -> ice::HeapString<>;

static constexpr ice::LogTagDefinition LogTag_Main = ice::create_log_tag(ice::LogTag::None, "hsc-packer");
static constexpr ice::LogTagDefinition LogTag_Details = ice::create_log_tag(LogTag_Main, "details");

static constexpr ice::ParamDefinition<ice::String> Param_Output{
    .name = "output",
    .name_short = "o",
    .description = "The hailstorm output file.",
    .flags = ice::ParamFlags::IsRequired,
};

static constexpr ice::ParamDefinition<ice::String> Param_Include{
    .name = "include",
    .name_short = "i",
    .description = "Adds a directory which will be searched for resource files.",
    .flags = ice::ParamFlags::IsRequired,
};

static constexpr ice::ParamDefinition<ice::String> Param_Config{
    .name = "config",
    .name_short = "c",
    .description = "Configuration file(s) with more detailed generation requirements.",
    .validator = hscp_param_validate_file,
};

static constexpr ice::ParamDefinition<bool> Param_Verbose{
    .name = "verbose",
    .name_short = "v",
    .description = "Detailed output during the packing process.",
    .flags = ice::ParamFlags::IsFlag,
};

#define HSCP_LOG(format, ...) ICE_LOG(ice::LogSeverity::Retail, LogTag_Main, format, __VA_ARGS__)
#define HSCP_ERROR(format, ...) ICE_LOG(ice::LogSeverity::Error, LogTag_Main, format, __VA_ARGS__)

#define HSCP_LOG_IF(condition, format, ...) ICE_LOG_IF(condition, ice::LogSeverity::Retail, LogTag_Main, format, __VA_ARGS__)
#define HSCP_ERROR_IF(condition, format, ...) ICE_LOG_IF(condition, ice::LogSeverity::Error, LogTag_Main, format, __VA_ARGS__)
