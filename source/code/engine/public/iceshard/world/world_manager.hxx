#pragma once

namespace iceshard::world
{


    class World;


    //! \brief A manager for world containers.
    class WorldManager
    {
    public:
        virtual ~WorldManager() noexcept = default;
    };


} // namespace iceshard::world
