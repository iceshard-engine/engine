#pragma once
#include <core/build/config.hxx>
#include <core/build/platform.hxx>

namespace core::build
{


//! \brief True if building for Debug.
static constexpr bool is_debug = configuration::current_config == configuration::ConfigurationType::Debug;

//! \brief True if building for ReleaseDebug.
static constexpr bool is_release_debug = configuration::current_config == configuration::ConfigurationType::ReleaseDebug;

//! \brief True if building for Release.
static constexpr bool is_release = configuration::current_config == configuration::ConfigurationType::Release;


//! \brief True if building for Windows.
static constexpr bool is_windows = platform::current_platform == platform::System::Windows;

//! \brief True if building for 64 bit.
static constexpr bool is_x64 = platform::current_platform == platform::Architecture::x64;

//! \brief True if building with MSVC.
static constexpr bool is_msvc = platform::current_platform == platform::Compiler::MSVC;


} // namespace core::build
