#pragma once
#include <core/collections.hxx>
#include <iceshard/renderer/render_api.hxx>

#include <cppcoro/task.hpp>

namespace iceshard
{

    class Frame;

    class Engine;

    class RenderPass;

    class RenderStage
    {
    public:
        virtual ~RenderStage() noexcept = default;

        virtual auto name() const noexcept -> core::stringid_type = 0;

        void add_system(core::stringid_arg_type name) noexcept
        {
            add_system_before(name, core::stringid_invalid);
        }

        virtual void add_system_before(core::stringid_arg_type name, core::stringid_arg_type before) noexcept = 0;

    protected:
        virtual void on_prepare(
            iceshard::Engine& engine,
            iceshard::RenderPass& render_pass
        ) noexcept = 0;

        virtual void on_execute(
            iceshard::Frame& current,
            iceshard::RenderPass& render_pass
        ) noexcept { }

        virtual void on_cleanup(
            iceshard::Engine& engine,
            iceshard::RenderPass& render_pass
        ) noexcept = 0;
    };

    class RenderStageTaskFactory
    {
    public:
        virtual ~RenderStageTaskFactory() noexcept = default;

        virtual void create_render_tasks(
            iceshard::Frame const& current,
            iceshard::renderer::api::CommandBuffer cmds,
            core::Vector<cppcoro::task<>>& task_list
        ) noexcept = 0;
    };

} // namespace iceshard
