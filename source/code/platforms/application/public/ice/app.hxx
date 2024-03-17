/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/expected.hxx>
#include <ice/param_list.hxx>
#include <ice/span.hxx>

namespace ice::app
{

    struct Config;
    struct State;
    struct Runtime;

    struct Factories
    {
        template<typename Type, typename... Args>
        using FactoryFn = auto(*)(ice::Allocator&, Args...) noexcept -> ice::UniquePtr<Type>;

        FactoryFn<ice::app::Config> factory_config;
        FactoryFn<ice::app::State> factory_state;
        FactoryFn<ice::app::Runtime> factory_runtime;



        template<typename T>
        static inline void destroy_default_object(T* obj) noexcept
        {
            obj->alloc.destroy(obj);
        }

        template<typename T>
        static inline auto create_default_object(ice::Allocator& alloc) noexcept -> ice::UniquePtr<T>
        {
            return ice::make_unique<T>(&destroy_default_object<T>, alloc.create<T>(alloc));
        }

        template<typename T>
        static inline auto create_default() noexcept -> FactoryFn<T>
        {
            return create_default_object<T>;
        }
    };

    static constexpr ice::ErrorCode S_ApplicationExit{ "S.0100:App:Requested 'Exit' stage" };
    static constexpr ice::ErrorCode S_ApplicationResume{ "S.0101:App:Requested 'Resume' stage" };
    static constexpr ice::ErrorCode S_ApplicationUpdate{ "S.0102:App:Requested 'Update' stage" };
    static constexpr ice::ErrorCode S_ApplicationSuspend{ "S.0103:App:Requested 'Suspend' stage" };
    static constexpr ice::ErrorCode S_ApplicationSetupPending{ "S.0104:App:Requested 'Setup' stage until setup finished." };

    static constexpr ice::ErrorCode E_FailedApplicationSetup{ "E.0100:App:Failed application 'Setup' stage" };
    static constexpr ice::ErrorCode E_FailedApplicationResume{ "E.0101:App:Failed application 'Resume' stage" };
    static constexpr ice::ErrorCode E_FailedApplicationUpdate{ "E.0102:App:Failed application 'Update' stage" };
    static constexpr ice::ErrorCode E_FailedApplicationSuspend{ "E.0103:App:Failed application 'Suspend' stage" };
    static constexpr ice::ErrorCode E_FailedApplicationShutdown{ "E.0104:App:Failed application 'Shutdown' stage" };

} // namespace ice::app

void ice_init(
    ice::Allocator& alloc,
    ice::app::Factories& factories
) noexcept;

void ice_args(
    ice::Allocator& alloc,
    ice::ParamList& params
) noexcept;

auto ice_setup(
    ice::Allocator& alloc,
    ice::ParamList const& params,
    ice::app::Config& config,
    ice::app::State& state
) noexcept -> ice::Result;

auto ice_resume(
    ice::app::Config const& config,
    ice::app::State& state,
    ice::app::Runtime& runtime
) noexcept -> ice::Result;

auto ice_update(
    ice::app::Config const& config,
    ice::app::State const& state,
    ice::app::Runtime& runtime
) noexcept -> ice::Result;

auto ice_suspend(
    ice::app::Config const& config,
    ice::app::State& state,
    ice::app::Runtime& runtime
) noexcept -> ice::Result;

auto ice_shutdown(
    ice::Allocator& alloc,
    ice::ParamList const& params,
    ice::app::Config const& config,
    ice::app::State& state
) noexcept -> ice::Result;
