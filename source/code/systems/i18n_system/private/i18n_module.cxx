/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/i18n_core_module.hxx>
#include <ice/i18n_module.hxx>
#include "i18n_resource_database.hxx"

namespace ice
{
    namespace detail
    {

        static I18NDatabase const* global_I18NDatabase = nullptr;

        static void set_i18n_global_database(ice::I18NDatabase const* resolver) noexcept
        {
            global_I18NDatabase = resolver;
        }

        static auto get_i18n_global_database() noexcept -> ice::I18NDatabase const*
        {
            return global_I18NDatabase;
        }

        static auto get_i18n_global_resolver() noexcept -> ice::I18NResolver const*
        {
            return global_I18NDatabase;
        }

    } // namespace detail

    void I18NModule::v1_i18n_api(ice::api::i18n::v1::I18NModuleAPI& api) noexcept
    {
        api.fn_set_i18n_database = ice::detail::set_i18n_global_database;
        api.fn_get_i18n_database = ice::detail::get_i18n_global_database;
    }

    void I18NModule::v1_i18n_core_api(ice::api::i18n::v1::I18NCoreModuleAPI& api) noexcept
    {
        api.fn_get_i18n_resolver = ice::detail::get_i18n_global_resolver;
    }

    bool I18NModule::on_load(ice::Allocator& alloc, ice::ModuleNegotiator auto const& negotiator) noexcept
    {
        bool result = true;
        if (negotiator.from_app())
        {
            result = negotiator.register_api(v1_i18n_core_api)
                && negotiator.register_api(v1_i18n_api);
        }
        I18NCoreModule::init(alloc, negotiator);
        return result;
    }

    void I18NModule::init(const Allocator& alloc, ice::ModuleNegotiatorBase const& negotiator) noexcept
    {
    }

    void I18NModule::on_unload(ice::Allocator& alloc) noexcept
    {
        // Clear the global pointer
        ice::detail::set_i18n_global_database(nullptr);
    }

} // namespace ice
