#pragma once
#include <core/cexpr/stringid.hxx>

namespace iceshard
{


    class World;


    //! \brief A manager for world containers.
    class WorldManager
    {
    public:
        virtual ~WorldManager() noexcept = default;

        //! \brief Returns a pointer to a world object with the given name.
        virtual auto get_world(core::cexpr::stringid_argument_type world_name) noexcept -> World* = 0;

        //! \brief Creates a new world object and returns a reference to it.
        virtual auto create_world(core::cexpr::stringid_argument_type world_name) noexcept -> World* = 0;

        //! \brief Destroys the world with the given name and all associated data.
        virtual void destroy_world(core::cexpr::stringid_argument_type world_name) noexcept = 0;
    };


} // namespace iceshard::world
