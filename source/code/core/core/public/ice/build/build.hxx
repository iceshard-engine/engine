/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/build/config.hxx>
#include <ice/build/platform.hxx>
#include <ice/build/validate.hxx>
#include <ice/build/warnings.hxx>
#include <ice/build/constants.hxx>

namespace ice::build
{

    static constexpr bool is_debug = current_config == Configuration::Debug;

    static constexpr bool is_develop = current_config == Configuration::Develop;

    static constexpr bool is_profile = current_config == Configuration::Profile;

    static constexpr bool is_release = current_config == Configuration::Release;


    static constexpr bool is_windows = current_platform == System::Windows;

    static constexpr bool is_linux = current_platform == System::Linux;

    static constexpr bool is_android = current_platform == System::Android;

    static constexpr bool is_webapp = current_platform == System::WebApp;

    static constexpr bool is_unix = is_linux || is_android || is_webapp;


    static constexpr bool is_x64 = current_platform == Architecture::x86_x64 || current_platform == Architecture::Arm64;

    static constexpr bool is_arm = current_platform == ArchFamily::ARM;

    static constexpr bool is_msvc = current_platform == Compiler::MSVC;

    static constexpr bool is_clang = current_platform == Compiler::Clang;

    static constexpr bool is_gcc = current_platform == Compiler::GCC;


    static_assert(
        ISP_ARCH_BITS == (ice::build::is_x64 ? 64 : 32),
        "Missmatch on predicted architecture bit-size and `is_x64` built-time constant."
    );

} // namespace ice::build
