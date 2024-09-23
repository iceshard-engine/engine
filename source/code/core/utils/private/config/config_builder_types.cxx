/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "config_builder_utils.hxx"

namespace ice::config::detail
{

    ConfigBuilderContainer::ConfigBuilderContainer(ice::Allocator& alloc, ValType vtype) noexcept
        : ConfigBuilderEntry{ { .next = 0, .type = CONFIG_KEYTYPE_NONE, .vtype = vtype, .offset = 0, .size = 0 } }
        , _refcount{ 1 }
        , _entries{ alloc }
        , _keystrings{ alloc }
    {
        // Since we are a container we point to ourselves.
        data.val_container = this;
    }

    auto ConfigBuilderContainer::addref() noexcept -> ConfigBuilderContainer*
    {
        return ++_refcount, this;
    }

    bool ConfigBuilderContainer::release(ice::Allocator& alloc) noexcept
    {
        if (_refcount -= 1; _refcount == 0)
        {
            this->~ConfigBuilderContainer();
            alloc.deallocate(this);
        }
        return _refcount == 0;
    }

    void ConfigBuilderContainer::clear() noexcept
    {
        ice::Allocator& _allocator = *_entries._allocator;
        for (ConfigBuilderEntry& entry : _entries)
        {
            cb_clear_value_type(_allocator, &entry);
        }
        ice::array::clear(_entries);
    }

    ConfigBuilderContainer::~ConfigBuilderContainer() noexcept
    {
        clear();
    }

} // namespace ice
