#pragma once
#include <core/allocator.hxx>
#include <core/cexpr/stringid.hxx>

#include <iceshard/entity/entity.hxx>
#include <iceshard/component/service_provider.hxx>

namespace iceshard::world
{


    //! \brief A single world container which is allowed to hold component managers.
    class World
    {
    public:
        World(
            core::cexpr::stringid_argument_type world_name,
            iceshard::entity::entity_handle_type world_entity
        ) noexcept;

        virtual ~World() noexcept = default;

        //! \brief The world name.
        auto name() const noexcept -> core::cexpr::stringid_type;

        //! \brief The world entity.
        auto entity() const noexcept -> iceshard::entity::entity_handle_type;

        //! \brief The worlds service provider.
        virtual auto service_provider() noexcept -> component::ServiceProvider* = 0;

    protected:
        const core::cexpr::stringid_type _name;
        const iceshard::entity::entity_handle_type _entity;
    };


} // namespace iceshard
