#pragma once
#include <ice/stringid.hxx>

namespace ice::ui
{

    enum class ResourceType : ice::u32
    {
        None,
        Font,
        Utf8String,
        Texture,
    };

    struct ResourceInfo
    {
        ice::BaseStringID<true> id;
        ice::ui::ResourceType type;
        ice::u32 type_data;
    };

    struct UIResourceData
    {
        ice::ui::ResourceInfo info;
        void* location;
    };

} // namespace ice::ui
