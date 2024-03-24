/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/log.hxx>

#include <webgpu/webgpu.h>
#include <emscripten.h>
#undef assert

namespace ice::render::webgpu
{

    constexpr ice::LogTagDefinition LogTag_WebGPU = ice::create_log_tag(ice::LogTag::Module, "WebGPU");

#define ICE_LOG_WGPU(severity, message, ...) \
    ICE_LOG(severity, ice::render::webgpu::LogTag_WebGPU, message, __VA_ARGS__)

} // namespace ice::render::webgpu
