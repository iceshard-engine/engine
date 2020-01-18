#pragma once
#include <core/allocator.hxx>
#include <core/cexpr/stringid.hxx>

#include <iceshard/entity/entity.hxx>
#include <iceshard/service_provider.hxx>

namespace iceshard
{


    //! \brief A single world container which is allowed to hold component managers.
    class World
    {
    public:
        World(
            core::stringid_arg_type world_name,
            iceshard::entity_handle_type world_entity
        ) noexcept;

        virtual ~World() noexcept = default;

        //! \brief The world name.
        auto name() const noexcept -> core::stringid_type;

        //! \brief The world entity.
        auto entity() const noexcept -> iceshard::entity_handle_type;

        //! \brief The worlds service provider.
        virtual auto service_provider() noexcept -> iceshard::ServiceProvider* = 0;

    protected:
        core::stringid_type const _name;
        iceshard::entity_handle_type const _entity;
    };


} // namespace iceshard
