/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/module_types.hxx>
#include <concepts>

namespace ice::concepts
{

    template<typename T>
    concept APIType = requires(T t)
    {
        { T::Constant_APIName } -> std::convertible_to<ice::StringID_Arg>;
        { T::Constant_APIVersion } -> std::convertible_to<ice::u32>;
    };

    template<typename T>
    concept APIExplicitPriority = requires(T t)
    {
        { T::Constant_APIPriority } -> std::convertible_to<ice::u32>;
    };

    //! \note Only used for validating ModuleNegotiator concept.
    struct APIConceptStruct
    {
        static constexpr ice::StringID Constant_APIName = "api.concept.concept-api"_sid;
        static constexpr ice::u32 Constant_APIVersion = ice::u32_max;
    };

    template<typename T>
    concept ModuleNegotiator = requires(T const& t, ice::StringID_Arg id, ice::FnModuleSelectAPI* api) {
        { t.query_apis(id, ice::u32{}, (ice::ModuleAPI*) nullptr, (ice::u32*)nullptr) } -> std::convertible_to<bool>;
        { t.register_api(id, api) } -> std::convertible_to<bool>;
        { t.from_app() } -> std::convertible_to<bool>;
    } && requires(T const& t, ice::ProcAPIQuickRegisterFunc<APIConceptStruct> func) {
        { t.register_api(func) } -> std::convertible_to<bool>;
    };

    template<typename T>
    concept ModuleLoadable = requires(T t, ice::Allocator& alloc, ice::ModuleNegotiatorTagged<T> const& negotiator)
    {
        { T::on_load(alloc, negotiator) } -> std::convertible_to<bool>;
    };

    template<typename T>
    concept ModuleUnloadable = requires(T t, ice::Allocator& alloc)
    {
        { T::on_unload(alloc) } -> std::convertible_to<void>;
    };

    // Currently this can be seen as an alias.
    template<typename T>
    concept ModuleType = ModuleLoadable<T>;

} // namespace ice::concepts
