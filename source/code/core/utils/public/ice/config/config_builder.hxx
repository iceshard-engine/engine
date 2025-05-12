/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/config/config_types.hxx>
#include <ice/container/array.hxx>
#include <ice/string/heap_var_string.hxx>

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

    // inline auto configbuilder_root(ConfigBuilder& b, ice::Allocator& alloc) noexcept -> ice::Memory
    // {
    //     ConfigBuilderValue val = b["asd"];
    //     b["asd"][0][1];
    //     b["a"]["b"] = 31u;
    //     val = b["a"];
    //     val["c"][2] = ice::u8{42};
    //     b["b"] = 32;
    //     b["c"] = 33;
    //     val = b["asd"][0];//[0][1]["ad"];
    //     // b.reset();
    //     val = b["e"][3];
    //     b["e"][4];
    //     ice::HeapVarString<>& str = b["f"] = "Test string";
    //     str = "Maybe not?";
    //     b["my"]["holy"]["cow"] = 69.420;

    //     ice::Memory mem = b.finalize(alloc);

    //     ice::Config c = ice::config::from_data(ice::data_view(mem));
    //     [[maybe_unused]]
    //     ice::u32 f;
    //     ice::config::get<ice::u32>(c, "a.b", f);
    //     ice::String s = ice::config::get<ice::String>(c, "f").value();
    //     f = 23;

    //     alloc.deallocate(mem);
    //     return {};
    // }

} // namespace ice
