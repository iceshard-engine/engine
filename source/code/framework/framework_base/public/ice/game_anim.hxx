#pragma once
#include <ice/input/input_types.hxx>
#include <ice/unique_ptr.hxx>

namespace ice
{

    struct Animation
    {
        static constexpr ice::StringID Identifier = "ice.component.animation"_sid;

        ice::StringID_Hash animation;
        ice::f32 speed = 1.f / 60.f;
    };

    struct AnimationState
    {
        static constexpr ice::StringID Identifier = "ice.component.animation-state"_sid;

        ice::StringID_Hash current_animation;
        ice::i64 timestamp;
        ice::vec2u frame;
    };

    class WorldTrait;

    auto create_trait_animator(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::WorldTrait>;

} // namespace ice
