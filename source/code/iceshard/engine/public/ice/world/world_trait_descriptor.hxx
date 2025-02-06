/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/world/world_trait_types.hxx>

namespace ice
{

    using TraitFactoryFn = auto(ice::Allocator& alloc, ice::TraitContext& ctx, void* userdata) noexcept -> ice::UniquePtr<ice::Trait>;
    using TraitTypeRegisterFn = bool(ice::Allocator& alloc, ice::EngineStateTracker& engine) noexcept;
    using TraitTypeUnregisterFn = void(ice::Allocator& alloc) noexcept;

    struct TraitDescriptor
    {
        ice::StringID name;
        ice::TraitFactoryFn* fn_factory;
        ice::TraitTypeRegisterFn* fn_register;
        ice::TraitTypeUnregisterFn* fn_unregister;
        ice::Span<ice::StringID const> required_dependencies;
        ice::Span<ice::StringID const> optional_dependencies;
        void* fn_factory_userdata = nullptr;
    };

} // namespace ice
