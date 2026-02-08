/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "config_internal.hxx"
#include <ice/config/config_builder.hxx>
#include <ice/heap_varstring.hxx>
#include <ice/heap_string.hxx>
#include <ice/array.hxx>

namespace ice::config::detail
{

    struct ConfigBuilderContainer;

    struct ConfigBuilderEntry : ConfigKey
    {
        ~ConfigBuilderEntry() noexcept = default;

        ConfigBuilderEntry(ConfigKey key) noexcept
            : ConfigKey{ key }
            , data{ .val_custom = { } }
        { }

        union Data
        {
            bool val_bool;
            ice::u8 val_u8;
            ice::u16 val_u16;
            ice::u32 val_u32;
            ice::u64 val_u64;

            ice::i8 val_i8;
            ice::i16 val_i16;
            ice::i32 val_i32;
            ice::i64 val_i64;

            ice::f32 val_f32;
            ice::f64 val_f64;

            ice::HeapVarString<>* val_varstr;
            ConfigBuilderContainer* val_container;
            struct
            {
                void const* location;
                ice::usize size;
            } val_custom;
        } data;
    };

    struct ConfigBuilderContainer : ConfigBuilderEntry
    {
        ConfigBuilderContainer(ice::Allocator& alloc, ValType vtype) noexcept;

        auto addref() noexcept -> ConfigBuilderContainer*;
        bool release(ice::Allocator& alloc) noexcept;
        void clear() noexcept;

        ice::u32 _refcount;
        ice::Array<ConfigBuilderEntry> _entries;
        ice::HeapString<> _keystrings;

    private:
        ~ConfigBuilderContainer() noexcept;
    };

} // namespace ice
