/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine_types.hxx>
#include <ice/engine_data_storage.hxx>
#include <ice/hashmap.hxx>

namespace ice
{

    struct IceshardDataStorage : ice::DataStorage
    {
        ice::Allocator& _backing;
        ice::Array<void*> _allocated;
        ice::HashMap<void*> _values;

        IceshardDataStorage(ice::Allocator& alloc, std::source_location const& source_location = std::source_location::current()) noexcept
            : ice::DataStorage{ source_location, "iceshard-data-storage" }
            , _backing{ alloc }
            , _allocated{ alloc }
            , _values{ alloc }
        {
        }

        ~IceshardDataStorage() noexcept
        {
            for (void* p : _allocated)
            {
                _backing.deallocate(p);
            }
        }

        bool has(ice::StringID_Arg name) const noexcept override
        {
            return _values.has(name);
        }

        bool set(ice::StringID_Arg name, void* value) noexcept override
        {
            ice::u64 const hash = ice::hash(name);
            bool const missing = _values.missing(hash);
            ICE_ASSERT_CORE(missing);
            //if (missing)
            {
                _values.set(ice::hash(name), value);
            }
            return missing;
        }

        bool get(ice::StringID_Arg name, void*& value) noexcept override
        {
            value = _values.get(name, nullptr);
            return value != nullptr;
        }

        bool get(ice::StringID_Arg name, void const*& value) const noexcept override
        {
            value = _values.get(name, nullptr);
            return value != nullptr;
        }

        auto allocate_named(ice::StringID_Arg name, ice::AllocRequest alloc_request) noexcept -> ice::AllocResult override
        {
            ice::AllocResult result = this->allocate(alloc_request);
            if (this->set(name, result.memory) == false)
            {
                this->deallocate(ice::exchange(result, {}));
            }
            return result;
        }

    protected: // Implementation of: ice::Allocator
        auto do_allocate(ice::AllocRequest request) noexcept -> ice::AllocResult override
        {
            ice::AllocResult const r = _backing.allocate(request);
            _allocated.push_back(r.memory);
            return r;
        }

        void do_deallocate(void* pointer) noexcept override
        {
            return _backing.deallocate(pointer);
        }
    };

} // namespace ice
