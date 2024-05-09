/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/log.hxx>
#include <ice/log_tag.hxx>
#include <ice/mem_allocator.hxx>
#include <ice/string/string.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/string_utils.hxx>
#include <ice/native_file.hxx>
#include <ice/params.hxx>

auto hscp_process_directory(ice::Allocator& alloc, ice::String dir) noexcept -> ice::HeapString<>;

static constexpr ice::LogTagDefinition LogTag_Main = ice::create_log_tag(ice::LogTag::None, "hsc-packer");
static constexpr ice::LogTagDefinition LogTag_Details = ice::create_log_tag(LogTag_Main, "details");

static constexpr ice::ParamDefinition Param_Output{
    .name = "-o,--output",
    .description = "The hailstorm output file.",
    .flags = ice::ParamFlags::IsRequired,
};

static constexpr ice::ParamDefinition Param_Include{
    .name = "-i,--include",
    .description = "Adds a directory which will be searched for resource files.",
    .flags = ice::ParamFlags::IsRequired,
};

static constexpr ice::ParamDefinition Param_Config{
    .name = "-c,--config",
    .description = "Configuration file(s) with more detailed generation requirements.",
};

static constexpr ice::ParamDefinition Param_Verbose{
    .name = "-v,--verbose",
    .description = "Detailed output during the packing process.",
};

#define HSCP_LOG(format, ...) ICE_LOG(ice::LogSeverity::Retail, LogTag_Main, format, __VA_ARGS__)
#define HSCP_ERROR(format, ...) ICE_LOG(ice::LogSeverity::Error, LogTag_Main, format, __VA_ARGS__)

#define HSCP_LOG_IF(condition, format, ...) ICE_LOG_IF(condition, ice::LogSeverity::Retail, LogTag_Main, format, __VA_ARGS__)
#define HSCP_ERROR_IF(condition, format, ...) ICE_LOG_IF(condition, ice::LogSeverity::Error, LogTag_Main, format, __VA_ARGS__)
