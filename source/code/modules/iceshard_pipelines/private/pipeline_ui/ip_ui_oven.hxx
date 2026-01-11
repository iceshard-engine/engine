/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include "ip_ui_oven_types.hxx"
#include <ice/string.hxx>
#include <ice/array.hxx>
#include <ice/ui_types.hxx>

#include <rapidxml_ns/rapidxml_ns.hpp>
#undef assert

namespace ice
{

    using ice::ui::ElementType;

    struct RawElement;
    struct RawShard;
    struct RawResource;
    struct RawStyle;

    static constexpr ice::String Constant_ISUINamespaceUI = "https://www.iceshard.net/docs/engine/v1_alpha/isui/ui/";
    static constexpr ice::String Constant_ISUINamespaceIceShard = "https://www.iceshard.net/docs/engine/v1_alpha/isui/iceshard/";

    static constexpr ice::String Constant_UIElement_Root = "isui";

    void parse_ui_file(
        ice::Allocator& alloc,
        rapidxml_ns::xml_document<char>& doc,
        ice::Array<ice::RawElement>& raw_elements,
        ice::Array<ice::RawResource>& ui_resources,
        ice::Array<ice::RawShard>& raw_shards,
        ice::Array<ice::RawStyle>& raw_styles
    ) noexcept;

} // namespace ice
