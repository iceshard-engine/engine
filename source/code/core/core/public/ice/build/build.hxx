#pragma once
#include <ice/build/config.hxx>
#include <ice/build/platform.hxx>

namespace ice::build
{

    static constexpr bool is_debug = current_config == Configuration::Debug;

    static constexpr bool is_develop = current_config == Configuration::Develop;

    static constexpr bool is_profile = current_config == Configuration::Profile;

    static constexpr bool is_release = current_config == Configuration::Release;


    static constexpr bool is_windows = current_platform == System::Windows;

    static constexpr bool is_unix = current_platform == System::Unix;

    static constexpr bool is_x64 = current_platform == Architecture::x64;

} // namespace ice::build
