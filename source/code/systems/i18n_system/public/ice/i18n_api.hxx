#pragma once
#include <ice/i18n_core_api.hxx>
#include <ice/i18n_database.hxx>
#include <ice/i18n_resolver.hxx>

namespace ice::api::i18n::v1
{

    using FnSetI18NDatabase = auto(*)(ice::I18NDatabase const* resolver) noexcept -> void;
    using FnGetI18NDatabase = auto(*)() noexcept -> ice::I18NDatabase const*;

    struct I18NModuleAPI : I18NCoreModuleAPI
    {
        static constexpr ice::StringID Constant_APIName = "ice.i18n"_sid;
        static constexpr ice::u32 Constant_APIVersion = 1;

        FnSetI18NDatabase fn_set_i18n_database = nullptr;
        FnGetI18NDatabase fn_get_i18n_database = nullptr;
    };

} // ice::api::i18n
