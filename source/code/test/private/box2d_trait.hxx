#pragma once
#include <ice/input_action.hxx>
#include <ice/shard_payloads.hxx>
#include <ice/world/world_trait.hxx>
#include <box2d/box2d.h>

namespace ice
{

    class Box2DTrait final
        : public ice::Trait
        , public ice::TraitDevUI
        , public ice::InterfaceSelectorOf<ice::Box2DTrait, ice::TraitDevUI>
    {
    public:
        Box2DTrait(ice::TraitContext& context, ice::Allocator& alloc) noexcept;

        auto activate(ice::WorldStateParams const& world_update) noexcept -> ice::Task<> override;
        auto deactivate(ice::WorldStateParams const& world_update) noexcept -> ice::Task<> override;

        auto on_click(ice::InputAction const& action) noexcept -> ice::Task<>;
        auto on_update(ice::EngineFrameUpdate const& update) noexcept -> ice::Task<>;

    public: // TraitDevUI
        auto trait_name() const noexcept -> ice::String override { return "Box2D.Physics"; }
        void build_content() noexcept override;

    private:
        b2WorldId _worldid;
    };

} // namespace ice
