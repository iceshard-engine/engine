/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string_types.hxx>
#include <ice/task_generator.hxx>
#include <arctic/arctic_syntax_node.hxx>
#include <arctic/arctic_syntax.hxx>

namespace ice
{

    namespace detail
    {

        template<typename U>
        inline auto arc_foreach(arctic::SyntaxNode<> node) noexcept -> ice::Generator<arctic::SyntaxNode<U>>
        {
            while (node)
            {
                arctic::SyntaxNode<> child = node.child();
                while(child)
                {
                    if (child.type() == U::RepresentedSyntaxEntity)
                    {
                        co_yield child.to<U>();
                    }
                    child = child.sibling();
                }
                node = node.sibling();
            }
            co_return;
        }

        inline bool arc_annotation(arctic::SyntaxNode<> node, arctic::String key, arctic::String& value) noexcept
        {
            for (arctic::SyntaxNode<arctic::syntax::AnnotationAttrib> annotation : arc_foreach<arctic::syntax::AnnotationAttrib>(node.annotation()))
            {
                arctic::syntax::AnnotationAttrib const& attrib = annotation.data();
                if (attrib.name == key)
                {
                    value = attrib.value;
                    return true;
                }
            }
            return false;
        }

        constexpr auto arc_str(arctic::String str) noexcept -> ice::String
        {
            return ice::String{ str.data(), static_cast<ice::ucount>(str.size()) };
        }

        constexpr auto arc_hash(arctic::String str) noexcept -> ice::u64
        {
            return ice::hash(arc_str(str));
        }

    } // namespace detail

} // namespace ice
