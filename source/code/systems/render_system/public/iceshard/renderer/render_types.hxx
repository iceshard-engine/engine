#pragma once
#include <iceshard/renderer/render_api.hxx>

namespace iceshard::renderer
{

    using RenderPass = iceshard::renderer::api::RenderPass;

    enum class RenderPassFeatures : uint32_t;

    enum class RenderPassStage : uint32_t;

    using CommandBuffer = iceshard::renderer::api::CommandBuffer;

} // namespace iceshard::renderer
