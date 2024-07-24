/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/stringid.hxx>
#include <ice/task_types.hxx>

namespace ice
{

    struct DataStorage;

    struct Engine;
    struct EngineCreateInfo;
    struct EngineDevUI;
    struct EngineFrame;
    struct EngineFrameData;
    struct EngineFrameSchedulers;
    struct EngineFrameUpdate;
    struct EngineRunner;
    struct EngineRunnerCreateInfo;
    struct EngineStateTracker;

    struct EngineSchedulers
    {
        ice::TaskScheduler& main;
        ice::TaskScheduler& tasks;
    };

    struct Trait;
    struct TraitArchive;
    struct TraitContext;
    struct TraitDescriptor;
    struct TraitTaskBinding;
    struct TraitTaskRegistry;

    struct World;
    struct WorldAssembly;
    struct WorldStateParams;
    struct WorldUpdater;

    namespace concepts
    {

        template<typename T>
        concept NamedDataType = requires(T t) {
            { ice::clear_type_t<T>::Identifier } -> std::convertible_to<ice::StringID const>;
        } && std::is_trivially_destructible_v<T>;

    } // namespace concepts

    namespace detail
    {

        template<typename R, typename T = R>
        auto map_task_arg(T value) noexcept -> R
        {
            return static_cast<R>(value);
        }

        template<typename Target>
        struct ArgMapper
        {
            template<typename Source>
            static auto select(Source&& source) noexcept -> Target { return ice::detail::map_task_arg<Target, decltype(source)>(source); }
        };

    } // namespace details

} // namespace ice
