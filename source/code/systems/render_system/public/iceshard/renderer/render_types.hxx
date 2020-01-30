#pragma once
#include <iceshard/renderer/render_api.hxx>

namespace iceshard::renderer
{

    using RenderPass = iceshard::renderer::api::RenderPass;

    enum class RenderPassFeatures : uint32_t;

    enum class RenderPassStage : uint32_t;

    using RenderPipeline = iceshard::renderer::api::RenderPipeline;

    enum class RenderPipelineLayout : uint32_t;

    using CommandBuffer = iceshard::renderer::api::CommandBuffer;

    using ResourceSet = iceshard::renderer::api::ResourceSet;

    struct RenderResource;

    using Texture = iceshard::renderer::api::Texture;

    using Sampler = iceshard::renderer::api::Sampler;

} // namespace iceshard::renderer
