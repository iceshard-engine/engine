#pragma once
#include <core/cexpr/stringid.hxx>
#include <iceshard/entity/entity.hxx>
#include <iceshard/component/component_block.hxx>

namespace iceshard
{

    class Frame;

    class RenderStageTaskFactory;

    //! \brief A regular interface for component systems.
    class ComponentSystem
    {
    public:
        virtual ~ComponentSystem() noexcept = default;

        virtual void update(Frame& frame, Frame const& previous_frame) noexcept;

        virtual void update(Frame& frame) noexcept;

        virtual void end_frame(Frame& frame, Frame const& previous_frame) noexcept;

        virtual void end_frame(Frame& frame) noexcept;

        virtual auto render_task_factory() noexcept -> RenderStageTaskFactory* { return nullptr; }
    };

} // namespace iceshard
