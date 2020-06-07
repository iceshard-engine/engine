#include "ice_render_stage.hxx"
#include "../ice_render_pass.hxx"

#include <iceshard/engine.hxx>
#include <iceshard/frame.hxx>
#include <iceshard/service_provider.hxx>
#include <iceshard/component/component_system.hxx>

#include <cppcoro/when_all.hpp>
#include <cppcoro/sync_wait.hpp>

namespace iceshard
{
    IceRenderStage::IceRenderStage(core::allocator& alloc) noexcept
        : _systems{ alloc }
    {
    }

    void IceRenderStage::prepare(iceshard::IceRenderPass& rp) noexcept
    {
        on_prepare(rp.engine(), rp);
    }

    void IceRenderStage::cleanup(iceshard::IceRenderPass& rp) noexcept
    {
        on_cleanup(rp.engine(), rp);
    }

    void IceRenderStage::execute(
        Frame& current,
        iceshard::IceRenderPass& render_pass
    ) noexcept
    {
        on_execute(current, render_pass);
    }

    void IceRenderStage::on_execute(
        iceshard::Frame& current,
        iceshard::RenderPass& render_pass
    ) noexcept
    {
        await_tasks(current, render_pass.command_buffer());
    }

    void IceRenderStage::add_system(core::stringid_arg_type name) noexcept
    {
        core::pod::array::push_back(_systems, name.hash_value);
    }

    void IceRenderStage::await_tasks(
        iceshard::Frame& current,
        iceshard::renderer::api::CommandBuffer cmds
    ) const noexcept
    {
        core::Vector<cppcoro::task<>> task_list{ current.frame_allocator() };
        task_list.reserve(20);

        auto& services = current.engine().services();
        for (auto system_name : _systems)
        {
            if (auto* system = services.component_system({ system_name }); system != nullptr)
            {
                if (auto* factory = system->render_task_factory(); factory != nullptr)
                {
                    factory->create_render_tasks(current, cmds, task_list);
                }
            }
        };

        for (auto& task : task_list)
        {
            cppcoro::sync_wait(task);
        }
    }

} // namespace iceshard
