#pragma once
#include <core/build/config.hxx>
#include <core/build/platform.hxx>

namespace core::build
{

//! \brief Is Debug configuration.
static constexpr bool is_debug = configuration::current_config == configuration::ConfigurationType::Debug;

//! \brief Is ReleaseDebug configuration.
static constexpr bool is_release_debug = configuration::current_config == configuration::ConfigurationType::ReleaseDebug;

//! \brief Is Release configuration.
static constexpr bool is_release = configuration::current_config == configuration::ConfigurationType::Release;


//! \brief Building for windows.
static constexpr bool is_windows = platform::current_platform == platform::System::Windows;

//! \brief Building for 64 bit architecture.
static constexpr bool is_x64 = platform::current_platform == platform::Architecture::x64;

//! \brief Building with Microsoft compiler.
static constexpr bool is_msvc = platform::current_platform == platform::Compiler::MSVC;

} // namespace core::build
