/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/devui_widget.hxx>
#include <ice/engine_types.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/shard_container.hxx>
#include <ice/span.hxx>
#include <ice/task.hxx>
#include <ice/world/world_trait_context.hxx>

namespace ice
{

    struct Trait
    {
        Trait(ice::TraitContext& context) noexcept;
        virtual ~Trait() noexcept = default;

        virtual auto activate(ice::WorldStateParams const& world_update) noexcept -> ice::Task<> { co_return; }
        virtual auto deactivate(ice::WorldStateParams const& world_update) noexcept -> ice::Task<> { co_return; }

    protected:
        ice::TraitContext& _context;
    };

    class TraitDevUI : public ice::DevUIWidget
    {
    public:
        using DevUIWidget::DevUIWidget;

        virtual auto trait_name() const noexcept -> ice::String = 0;

        static constexpr ice::StringID InterfaceID = "ice/interface/trait-devui"_sid;
    };

    static constexpr ice::Shard Shard_TraitSetup = "event/trait/setup`void"_shard;
    static constexpr ice::Shard Shard_TraitActivate = "event/trait/activate`void"_shard;
    static constexpr ice::Shard Shard_TraitUpdate = "event/trait/update`void"_shard;
    static constexpr ice::Shard Shard_TraitDeactivate = "event/trait/deactivate`void"_shard;
    static constexpr ice::Shard Shard_TraitShutdown = "event/trait/shutdown`void"_shard;

    static constexpr ice::StringID TraitID_GfxImageStorage = "trait.gfx-image-storage"_sid;
    static constexpr ice::StringID TraitID_GfxShaderStorage = "trait.gfx-shader-storage"_sid;

} // namespace ice
