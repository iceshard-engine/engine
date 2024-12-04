#pragma once
#include <ice/input_action.hxx>
#include <ice/shard_payloads.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/input_action_layer.hxx>

namespace ice
{

    class InputActionsTrait final
        : public ice::Trait
        , public ice::TraitDevUI
        , public ice::InterfaceSelectorOf<ice::InputActionsTrait, ice::TraitDevUI>
    {
    public:
        InputActionsTrait(ice::TraitContext& context, ice::Allocator& alloc) noexcept;

        auto activate(ice::WorldStateParams const& world_update) noexcept -> ice::Task<> override;
        auto deactivate(ice::WorldStateParams const& world_update) noexcept -> ice::Task<> override;

        auto on_click(ice::InputAction const& action) noexcept -> ice::Task<>;
        auto on_update(ice::EngineFrameUpdate const& update) noexcept -> ice::Task<>;

    public: // TraitDevUI
        auto trait_name() const noexcept -> ice::String override { return "Engine.InputActions"; }
        void build_content() noexcept override;

    private:
        ice::Allocator& _allocator;
        ice::InputActionStack* _stack;
        ice::UniquePtr<ice::InputActionLayer> _layer;
    };

} // namespace ice
