/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/task_types.hxx>
#include <ice/concept/strong_type_value.hxx>
#include <ice/result_codes.hxx>

namespace ice::app
{

    struct ArgumentsConfig { };
    struct Arguments
    {
        ice::app::ArgumentsConfig const& config;
        ice::Span<const char*> args;
    };

    struct Config;
    struct State;
    struct Runtime;

    struct Factories
    {
        template<typename Type, typename... Args>
        using FactoryFn = auto(*)(ice::Allocator&, Args...) -> ice::UniquePtr<Type>;

        FactoryFn<ice::app::Config> factory_config;
        FactoryFn<ice::app::State> factory_state;
        FactoryFn<ice::app::Runtime> factory_runtime;
    };

    static constexpr ice::ResCode S_ApplicationExit = ResCode::create(ResultSeverity::Success, "Requested 'Exit' stage");
    static constexpr ice::ResCode S_ApplicationResume = ResCode::create(ResultSeverity::Success, "Requested 'Resume' stage");
    static constexpr ice::ResCode S_ApplicationUpdate = ResCode::create(ResultSeverity::Success, "Requested 'Update' stage");
    static constexpr ice::ResCode S_ApplicationSuspend = ResCode::create(ResultSeverity::Success, "Requested 'Suspend' stage");

    static constexpr ice::ResCode E_FailedApplicationSetup = ResCode::create(ResultSeverity::Error, "Failed application 'Setup' stage");
    static constexpr ice::ResCode E_FailedApplicationResume = ResCode::create(ResultSeverity::Error, "Failed application 'Resume' stage");
    static constexpr ice::ResCode E_FailedApplicationUpdate = ResCode::create(ResultSeverity::Error, "Failed application 'Update' stage");
    static constexpr ice::ResCode E_FailedApplicationSuspend = ResCode::create(ResultSeverity::Error, "Failed application 'Suspend' stage");
    static constexpr ice::ResCode E_FailedApplicationShutdown = ResCode::create(ResultSeverity::Error, "Failed application 'Shutdown' stage");

} // namespace ice::app

void ice_init(
    ice::Allocator& alloc,
    ice::app::Factories& factories
) noexcept;

void ice_args(
    ice::Allocator& alloc,
    ice::app::ArgumentsConfig& args
) noexcept;

auto ice_setup(
    ice::Allocator& alloc,
    ice::app::Arguments const& args,
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
    ice::app::Arguments const& args,
    ice::app::Config const& config,
    ice::app::State& state
) noexcept -> ice::Result;
