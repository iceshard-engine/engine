#pragma once
#include <ice/stringid.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/world/world_trait.hxx>

namespace ice
{

    static constexpr ice::StringID Constant_TraitName_PhysicsBox2D
        = "ice.base-framework.trait-physics-box2d"_sid;

    static constexpr ice::f32 Constant_PixelsInMeter = 64.f;

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

        virtual auto create_static_body(
            ice::vec2f position,
            ice::u32 vertice_count,
            ice::vec2f const* vertices
        ) noexcept -> ice::PhysicsID = 0;

        virtual void destroy_body(
            ice::PhysicsID physics_id
        ) noexcept = 0;
    };

} // namespace ice
