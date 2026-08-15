/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/i18n_core_api.hxx>
#include <ice/module.hxx>

namespace ice
{

    struct I18NString;

    struct I18NCoreModule : public ice::Module<I18NCoreModule>
    {
        static void v1_i18n_api(ice::api::i18n::v1::I18NCoreModuleAPI& api) noexcept;

        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator auto const& negotiator) noexcept;
        static void on_unload(ice::Allocator& alloc) noexcept;

        static void init(const Allocator& alloc, ice::ModuleNegotiatorBase const& negotiator) noexcept;

        static auto resolve(ice::I18NReference const& reference) noexcept -> ice::String;
        static auto resolve(ice::I18NReference const& reference, fmt::format_args const& fmt_args) noexcept -> ice::String;
        static void resolve(ice::I18NString& text, ice::I18NReference const& reference) noexcept;

        IS_WORKAROUND_MODULE_INITIALIZATION(I18NCoreModule);
    };

} // namespace ice
