/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "i18n_resource_database.hxx"
#include <ice/config.hxx>
#include <ice/config/config_builder.hxx>
#include <ice/i18n_string.hxx>
#include <ice/mem_allocator_utils.hxx>
#include <ice/resource_filter.hxx>
#include <ice/task_utils.hxx>

namespace ice
{
    namespace detail
    {
        static ice::UniquePtr<ice::L10NStringsTable> null_ptr{};

        struct I10NStringsFilter final : ice::ResourceFilter
        {
            bool allows_resource(const ice::Resource* resource) const noexcept override
            {
                return resource->uri().path().extension() == ".isl10n";
            }
        };

    } // namespace detail

    I18NResourceDatabase::I18NResourceDatabase(
        ice::Allocator& alloc,
        ice::ResourceTracker& resources
    ) noexcept
        : _allocator{ alloc, "I18N" }
        , _resources{ resources }
        , _default_language{ _allocator, "en" }
        , _languages{ _allocator }
        , _dynamic_entries{ _allocator }
    {
        _languages.reserve(10);
    }

    I18NResourceDatabase::~I18NResourceDatabase() noexcept
    {
        for (I18NDynamicEntry* entry : _dynamic_entries)
        {
            _allocator.destroy(entry);
        }
    }

    void I18NResourceDatabase::load_available_languages() noexcept
    {
        ice::detail::I10NStringsFilter filter;
        ice::Array<ice::URI> resource_uris{ _allocator };
        if (ice::wait_for_expected(_resources.filter_resource_uris(filter, resource_uris)) > 0)
        {
            for (ice::URI const& uri : resource_uris)
            {
                ice::ResourceHandle const handle = _resources.find_resource(uri);
                ice::ResourceResult const loaded = ice::wait_for_result(_resources.load_resource(handle));
                if (loaded.resource_status == ice::ResourceStatus::Loaded)
                {
                    ice::Path const path = uri.path();

                    // TODO: Replace this manual work with an proper Asset definition. Might require work on initialization order.
                    // Prepare the loaded data
                    ice::Memory const loaded_copy = ice::data_copy(_allocator, loaded.data);

                    // We use 'filename' so the refpath is not stripped. It's contains '.' so it's last part can be seen as an extension.
                    ice::String const refpath = path.directory().filename();
                    ice::String const lang = path.basename();
                    if (_languages.missing(lang))
                    {
                        _languages.set(lang, ice::make_unique<ice::L10NStringsTable>(_allocator, lang, _allocator));
                    }

                    ice::UniquePtr<ice::L10NStringsTable> const& strings = _languages.get(lang, ice::detail::null_ptr);
                    ICE_ASSERT_CORE(strings != nullptr);

                    // Store the resource handle
                    strings->add_resource(loaded_copy);

                    ice::u32 const path_hash = ice::hash32(refpath);
                    ice::String contents = ice::string_from_data<char>(loaded_copy);
                    while (contents.not_empty())
                    {
                        ice::nindex const line_end = contents.find_first_of('\n');
                        ice::L10NString const l10n_entry = L10NString::from_java_property(
                            contents.substr(0, line_end)
                        );
                        if (l10n_entry.value.not_empty())
                        {
                            strings->store(l10n_entry, path_hash);
                        }
                        contents = contents.substr(line_end.value_or(contents.size()) + 1);
                    }

                    // Replace newline characters with '\0'
                    char* const loaded_string = static_cast<char*>(loaded_copy.location);
                    for (ice::usize::base_type idx = 0; idx < loaded_copy.size.value; ++idx)
                    {
                        if (loaded_string[idx] == '\n' || loaded_string[idx] == '\r')
                        {
                            loaded_string[idx] = '\0';
                        }
                    }
                }
            }
        }
    }

    void I18NResourceDatabase::set_default_language(ice::String lang) noexcept
    {
        _default_language = lang;

        // Get the strings of the default language and build the dynamic entries map.
        ice::UniquePtr<ice::L10NStringsTable> const& strings = _languages.get(_default_language, ice::detail::null_ptr);
        if (strings != nullptr)
        {
            strings->for_each([this](ice::L10NString const& localized_string, ice::u64 full_hash)
            {
                I18NDynamicEntry* const entry = _dynamic_entries.get(full_hash, nullptr);
                if (entry == nullptr)
                {
                    _dynamic_entries.set(
                        full_hash,
                        _allocator.create<I18NDynamicEntry>(
                            localized_string.value.size(),
                            localized_string.value.data()
                        )
                    );
                }
                else
                {
                    entry->localized_count = localized_string.value.size();
                    entry->localized_data = localized_string.value.data();
                }
            });
        }
    }

    auto I18NResourceDatabase::resolve(ice::I18NReference key) const noexcept -> ice::String
    {
        ice::UniquePtr<ice::L10NStringsTable> const& strings = _languages.get(_default_language, ice::detail::null_ptr);
        if (strings != nullptr)
        {
            return strings->find(key);
        }
        return key.fallback();
    }

    auto I18NResourceDatabase::resolve(ice::I18NReference key, const fmt::format_args& args) const noexcept -> ice::String
    {
        ice::String lang = key.flags().language(args);
        if (lang.is_empty())
        {
            lang = _default_language;
        }

        ice::UniquePtr<ice::L10NStringsTable> const& strings = _languages.get(lang, ice::detail::null_ptr);
        if (strings != nullptr)
        {
            return strings->find(key);
        }
        return key.fallback();
    }

    void I18NResourceDatabase::resolve(ice::I18NString& inout_text, ice::I18NReference const& ref) const noexcept
    {
        ice::u64 const key_hash = ice::hash32(ref.key());
        ice::u64 const full_hash = (key_hash << 32) | ice::hash32(ref.path());

        // Only resolve this reference if it's actually valid.
        if (ref._hash != 0 && _dynamic_entries.has(full_hash))
        {
            ice::I18NDynamicEntry const* const entry = _dynamic_entries.get(full_hash, nullptr);
            ICE_ASSERT_CORE(entry != nullptr);

            inout_text._data = entry;
            inout_text._data_offset = 0;
            inout_text._size_offset = -8;
        }
    }

} // namespace ice
