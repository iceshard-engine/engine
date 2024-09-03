/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/config.hxx>
#include <ice/container/array.hxx>
#include <ice/string/heap_var_string.hxx>

namespace ice
{

    struct ConfigBuilderValue
    {
        struct Entry;

        ConfigBuilderValue(ice::Allocator* alloc, Entry* entry, ice::u32 ref) noexcept;
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

        template<typename T> requires std::is_trivial_v<T>
        auto operator=(T value) noexcept -> decltype(set(T{}));

        void reset();

        ice::Allocator* _alloc;
        Entry* _internal;
        ice::u32 _idx;
    };

    template<typename T> requires std::is_trivial_v<T>
    auto ConfigBuilderValue::operator=(T value) noexcept -> decltype(set(T{}))
    {
        return this->set(value);
    }

    class ConfigBuilder : public ConfigBuilderValue
    {
    public:
        ConfigBuilder(ice::Allocator& alloc) noexcept;
        ~ConfigBuilder() noexcept;

        auto finalize(ice::Allocator& alloc) noexcept -> ice::Memory;
    };

} // namespace ice
