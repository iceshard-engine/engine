#pragma once
#include <iceshard/renderer/render_system.hxx>
#include <iceshard/render/render_stage.hxx>

#include <core/pod/collections.hxx>

namespace iceshard
{

    class Frame;

    class Engine;

    class IceRenderPass;

    class IceRenderStage : public RenderStage
    {
    public:
        IceRenderStage(
            core::allocator& alloc
        ) noexcept;

        virtual ~IceRenderStage() noexcept = default;

        void prepare(
            iceshard::IceRenderPass& render_pass
        ) noexcept;

        void cleanup(
            iceshard::IceRenderPass& render_pass
        ) noexcept;

        void execute(
            iceshard::Frame& current,
            iceshard::IceRenderPass& render_pass
        ) noexcept;

        void on_execute(
            iceshard::Frame& current,
            iceshard::RenderPass& render_pass
        ) noexcept override;

        void await_tasks(
            iceshard::Frame& current,
            iceshard::renderer::api::CommandBuffer cmds
        ) const noexcept;

        void add_system_before(core::stringid_arg_type name, core::stringid_arg_type before) noexcept;

    protected:
        core::pod::Array<core::stringid_hash_type> _systems;
    };

} // namespace iceshard
