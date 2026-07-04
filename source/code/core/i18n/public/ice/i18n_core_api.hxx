#pragma once
#include <ice/i18n_resolver.hxx>

namespace ice::api::i18n::v1
{

    using FnGetI18NResolver = auto(*)() noexcept -> ice::I18NResolver const*;
    using FnSetI18NResolver = void(*)(ice::I18NResolver const* resolver) noexcept;

    struct I18NCoreModuleAPI
    {
        static constexpr ice::StringID Constant_APIName = "ice.i18n-core"_sid;
        static constexpr ice::u32 Constant_APIVersion = 1;

        FnGetI18NResolver fn_get_i18n_resolver = nullptr;
    };

} // namespace ice::api::i18n::v1
