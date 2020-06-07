#pragma once
#include <iceshard/renderer/render_api.hxx>

namespace iceshard
{

    class RenderPass;

    class RenderSystem
    {
    public:
        virtual ~RenderSystem() noexcept = default;

        virtual auto render_pass(core::stringid_arg_type name) noexcept -> iceshard::RenderPass* = 0;
    };

} // namespace iceshard
