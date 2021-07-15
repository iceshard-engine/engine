#pragma once
#include <ice/allocator.hxx>

namespace ice
{

    template<typename T>
    class Task;

    class DataStorage;
    class EntityStorage;
    class World;

    class WorldPortal
    {
    public:
        virtual ~WorldPortal() noexcept = default;

        virtual auto world() const noexcept -> ice::World const& = 0;

        virtual auto allocator() noexcept -> ice::Allocator& = 0;

        virtual auto storage() noexcept -> ice::DataStorage& = 0;
        virtual auto entity_storage() noexcept -> ice::EntityStorage& = 0;

        virtual void execute(ice::Task<void> task) noexcept = 0;
    };

} // namespace ice
