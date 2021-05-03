#include <ice/phx/phx_trait.hxx>

namespace ice::phx
{

    void PhxWorldTrait::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::World& world
    ) noexcept
    {
    }

    void PhxWorldTrait::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::World& world
    ) noexcept
    {
    }

    void PhxWorldTrait::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::World& world
    ) noexcept
    {
    }

} // namespace ice::phx
