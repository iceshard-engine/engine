#pragma once
#include <core/cexpr/stringid.hxx>
#include <iceshard/entity/entity.hxx>
#include <iceshard/component/component_block.hxx>

namespace iceshard
{

    class Engine;

    class World;

    //! \brief A regular interface for component systems.
    class ComponentSystem
    {
    public:
        virtual ~ComponentSystem() noexcept = default;

        virtual void update(Engine& engine) noexcept = 0;
    };

} // namespace iceshard
