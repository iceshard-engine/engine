/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <ice/math.hxx>

namespace ice::render
{

    class RenderDriver;
    class RenderDevice;
    class RenderSurface;
    class RenderSwapchain;
    class RenderQueue;
    class RenderCommands;
    class RenderFence;

    struct SurfaceInfo;
    struct RenderpassInfo;
    struct ResourceSetLayoutBinding;
    struct ResourceSetUpdateInfo;
    struct PipelineLayoutInfo;
    struct PipelineInfo;
    struct QueueFamilyInfo;
    struct QueueInfo;
    struct ShaderInfo;
    struct ImageInfo;
    struct ImageBarrier;
    struct SamplerInfo;
    struct BufferUpdateInfo;

    enum class [[nodiscard]] Framebuffer : ice::uptr;
    enum class [[nodiscard]] Renderpass : ice::uptr;
    enum class [[nodiscard]] ResourceSetLayout : ice::uptr;
    enum class [[nodiscard]] ResourceSet : ice::uptr;
    enum class [[nodiscard]] PipelineLayout : ice::uptr;
    enum class [[nodiscard]] Pipeline : ice::uptr;
    enum class [[nodiscard]] Shader : ice::uptr;
    enum class [[nodiscard]] Image : ice::uptr;
    enum class [[nodiscard]] Sampler : ice::uptr;
    enum class [[nodiscard]] Buffer : ice::uptr;
    enum class [[nodiscard]] CommandBuffer : ice::uptr;
    enum class [[nodiscard]] Semaphore : ice::uptr;

    enum class QueueID : ice::u32;
    enum class QueueFlags : ice::u32;
    enum class ImageFormat : ice::u32;
    enum class ImageLayout : ice::u32;
    enum class AttachmentType : ice::u32;
    enum class AccessFlags : ice::u32;
    enum class BufferType : ice::u32;
    enum class PipelineStage : ice::u32;
    enum class ShaderStageFlags : ice::u32;
    enum class CommandBufferType : ice::u32;
    enum class BarrierDependency : ice::u32;

    using ice::operator|;
    using ice::operator|=;
    using ice::operator&;
    using ice::operator&=;
    using ice::operator~;

} // namespace ice::render
