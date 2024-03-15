#pragma once
#include <ice/stringid.hxx>
#include <ice/engine_types.hxx>

namespace ice
{

    struct DataStorage
    {
        virtual bool set(ice::StringID name, void* value) noexcept = 0;
        virtual bool get(ice::StringID name, void*& value) noexcept = 0;
        virtual bool get(ice::StringID name, void const*& value) const noexcept = 0;
    };

} // namespace ice
