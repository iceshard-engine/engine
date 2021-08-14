#pragma once
#include <ice/stringid.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/world/world_trait.hxx>

namespace ice
{

    static constexpr ice::f32 Constant_PixelsInMeter = 96.f;

    enum class PhysicsID : ice::u64
    {
        Invalid = 0x0
    };

    enum class PhysicsShape : ice::u32
    {
        Box,
        Capsule,
    };

    struct PhysicsBody
    {
        static constexpr ice::StringID Identifier = "ice.component.phx-body"_sid;

        ice::PhysicsShape shape;
        ice::vec2f dimensions;

        void* trait_data = nullptr;
    };

    struct PhysicsVelocity
    {
        static constexpr ice::StringID Identifier = "ice.component.phx-velocity"_sid;

        ice::vec2f velocity;
    };

    class WorldTrait_Physics2D : public ice::WorldTrait
    {
    public:
        virtual auto create_static_body(
            ice::vec2f position,
            ice::PhysicsShape shape,
            ice::vec2f dimensions
        ) noexcept -> ice::PhysicsID = 0;

        virtual void destroy_body(
            ice::PhysicsID physics_id
        ) noexcept = 0;
    };

    auto create_trait_physics(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::WorldTrait_Physics2D>;

} // namespace ice
