/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/i18n.hxx>
#include <ice/i18n_resolver.hxx>
#include <ice/mem_unique_ptr.hxx>



namespace ice
{

    class ResourceTracker;

    class I18NDatabase : public ice::I18NResolver
    {
    public:
        ~I18NDatabase() noexcept override = default;

        virtual void load_available_languages() noexcept = 0;
        virtual void set_default_language(ice::String lang) noexcept = 0;
    };

    auto create_i18n_database(
        ice::Allocator& allocator,
        ice::ResourceTracker& resources
    ) noexcept -> ice::UniquePtr<I18NDatabase>;

} // namespace ice
