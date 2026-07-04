#pragma once
#include <ice/module.hxx>
#include <ice/module_negotiator.hxx>
#include <ice/i18n_api.hxx>

namespace ice
{

    struct I18NString;

    struct I18NModule : public ice::Module<I18NModule>
    {
        static void v1_i18n_api(ice::api::i18n::v1::I18NModuleAPI& api) noexcept;
        static void v1_i18n_core_api(ice::api::i18n::v1::I18NCoreModuleAPI& api) noexcept;

        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator auto const& negotiator) noexcept;
        static void on_unload(ice::Allocator& alloc) noexcept;

        static void init(const Allocator& alloc, ice::ModuleNegotiatorBase const& negotiator) noexcept;

        IS_WORKAROUND_MODULE_INITIALIZATION(I18NModule);
    };

} // namespace ice
