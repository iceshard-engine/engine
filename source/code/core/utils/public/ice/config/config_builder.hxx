/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/array.hxx>
#include <ice/config/config_types.hxx>
#include <ice/heap_varstring.hxx>

namespace ice
{

    struct ConfigBuilderValue
    {
        using ConfigBuilderEntry = ice::config::detail::ConfigBuilderEntry;

        ConfigBuilderValue(ice::Allocator* alloc, ConfigBuilderEntry* entry, ice::u32 ref) noexcept;
        ~ConfigBuilderValue() noexcept;

        ConfigBuilderValue(ConfigBuilderValue&&) noexcept;
        ConfigBuilderValue(ConfigBuilderValue const&) noexcept;
        auto operator=(ConfigBuilderValue&&) noexcept -> ConfigBuilderValue&;
        auto operator=(ConfigBuilderValue const&) noexcept -> ConfigBuilderValue&;

        auto operator[](ice::String key) noexcept -> ConfigBuilderValue;
        auto operator[](ice::u32 idx) noexcept -> ConfigBuilderValue;

        template<typename T> requires std::is_trivial_v<T>
        auto set(T value) noexcept -> T&;

        auto set(ice::String value) noexcept -> ice::HeapVarString<>&;
        auto set(char const* value) noexcept -> ice::HeapVarString<>& { return set(ice::String{value}); }

        template<typename T> requires std::is_trivial_v<T> || std::is_same_v<T, ice::String>
        auto operator=(T value) noexcept -> decltype(set(T{}));

        void reset();

        ice::Allocator* _alloc;
        ConfigBuilderEntry* _internal;
        ice::u32 _idx;
    };

    template<typename T> requires std::is_trivial_v<T> || std::is_same_v<T, ice::String>
    auto ConfigBuilderValue::operator=(T value) noexcept -> decltype(set(T{}))
    {
        return this->set(value);
    }

    class ConfigBuilder : public ConfigBuilderValue
    {
    public:
        ConfigBuilder(ice::Allocator& alloc) noexcept;
        ConfigBuilder(ice::Allocator& alloc, ice::Config const& config) noexcept;
        ~ConfigBuilder() noexcept;

        auto merge(ice::String json) noexcept -> ice::ErrorCode;
        auto merge(ice::Config const& config) noexcept -> ice::ErrorCode;

        auto finalize(ice::Allocator& alloc) noexcept -> ice::Memory;
    };

    namespace config
    {

        auto from_json(ice::ConfigBuilder& builder, ice::String json) noexcept -> ice::ErrorCode;

    } // namespace config

} // namespace ice
