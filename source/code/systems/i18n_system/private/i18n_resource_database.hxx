/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/i18n_database.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/hashmap.hxx>

namespace ice
{

    struct L10NString
    {
        ice::String key;
        ice::String value;

        static auto from_java_property(ice::String string) noexcept -> ice::L10NString
        {
            ice::nindex const sep = string.find_first_of('=');
            ice::L10NString result{};
            if (sep.is_valid())
            {
                result.key = string.substr(0, sep);
                result.value = string.substr(sep + 1);
            }
            return result;
        }
    };

    class L10NStringsTable
    {
    public:
        L10NStringsTable(ice::String lang, ice::Allocator& allocator) noexcept
            : _allocator{ allocator }
            , _target{ lang }
            , _strings{ _allocator }
            , _resources{ _allocator }
        { }

        ~L10NStringsTable() noexcept
        {
            for (ice::Memory loaded_memory : _resources)
            {
                _allocator.deallocate(loaded_memory);
            }
        }

        void add_resource(ice::Memory memory) noexcept
        {
            _resources.push_back(memory);
        }
        // void add_resource(ice::ResourceHandle resource_handle) noexcept
        // {
        //     _resources.push_back(resource_handle);
        // }

        void store(ice::L10NString l10n_string, ice::u32 path_hash) noexcept
        {
            ice::u64 const key_hash = ice::hash32(l10n_string.key);
            ice::u64 const full_hash = (key_hash << 32) | path_hash;

            ICE_ASSERT_CORE(_strings.missing(full_hash));
            _strings.set(full_hash, l10n_string);
        }

        auto find(ice::I18NReference i18n_string) const noexcept -> ice::String
        {
            ice::u64 const key_hash = ice::hash32(i18n_string.key());
            ice::u64 const full_hash = (key_hash << 32) | ice::hash32(i18n_string.path());

            ice::L10NString const* l10n_value = _strings.find(full_hash);
            if (l10n_value == nullptr)
            {
                return i18n_string.fallback();
            }

            ICE_ASSERT_CORE(l10n_value->key == i18n_string.key());
            return l10n_value->value;
        }

        template<typename Fn>
        void for_each(Fn const& fn) noexcept
        {
            auto it = _strings.begin();
            auto const end = _strings.end();
            while (it != end)
            {
                fn(it.value(), it.key());
                ++it;
            }
        }

    private:
        ice::Allocator& _allocator;
        ice::String _target;
        ice::HashMap<ice::L10NString> _strings;
        ice::Array<ice::Memory> _resources;
        // ice::Array<ice::ResourceHandle> _resources;
    };

    struct I18NDynamicEntry
    {
        ice::ncount localized_count = ice::ncount_none;
        char const* localized_data = nullptr;

        auto get_string_data() const noexcept -> char const* { return reinterpret_cast<char const*>(this + 1); }
    };

    class I18NResourceDatabase final : public I18NDatabase
    {
    public:
        I18NResourceDatabase(
            ice::Allocator& alloc,
            ice::ResourceTracker& resources
        ) noexcept;

        ~I18NResourceDatabase() noexcept override;

        void load_available_languages() noexcept override;
        void set_default_language(ice::String lang) noexcept override;

        auto resolve(ice::I18NReference key) const noexcept -> ice::String override;
        auto resolve(ice::I18NReference key, const fmt::format_args& args) const noexcept -> ice::String override;
        void resolve(ice::I18NString& inout_text, ice::I18NReference const& ref) const noexcept override;

    private:
        ice::ProxyAllocator _allocator;
        ice::ResourceTracker& _resources;

        ice::HeapString<> _default_language;
        ice::HashMap<ice::UniquePtr<ice::L10NStringsTable>> _languages;
        ice::HashMap<ice::I18NDynamicEntry*> _dynamic_entries;
    };

} // namespace ice
