#pragma once
#include <ice/input/input_types.hxx>

namespace ice
{

    enum class ActorType : ice::u32
    {
        Static,
        Player,
    };

    struct Actor
    {
        static constexpr ice::StringID Identifier = "ice.component.actor"_sid;

        ice::ActorType type;
    };

    class WorldTrait;

    auto create_trait_actor(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::WorldTrait>;

} // namespace ice
