#include <iceshard/component/component_system.hxx>

namespace iceshard
{

    void ComponentSystem::update(Frame& frame, Frame const& /*previous_frame*/) noexcept
    {
        update(frame);
    }

    void ComponentSystem::update(Frame& /*frame*/) noexcept
    {
    }

    void ComponentSystem::end_frame(Frame& frame, Frame const& /*previous_frame*/) noexcept
    {
        end_frame(frame);
    }

    void ComponentSystem::end_frame(Frame& /*frame*/) noexcept
    {
    }

} // namespace iceshard
