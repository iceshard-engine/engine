#pragma once
#include <ice/stringid.hxx>
#include <ice/unique_ptr.hxx>

namespace ice
{

    static constexpr ice::f32 Constant_PixelsInMeter = 96.f;

    struct PhysicsBody
    {
        static constexpr ice::StringID Identifier = "ice.component.phx-body"_sid;

        void* trait_data = nullptr;
    };

    class WorldTrait;

    auto create_trait_physics(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::WorldTrait>;

} // namespace ice
