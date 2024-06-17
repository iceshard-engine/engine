#include "shader_tools_asl_script.hxx"
#include "shader_tools_asl_utils.hxx"
#include <ice/profiler.hxx>

namespace ice
{

    ASLScriptFile::ASLScriptFile(ice::ASLAllocator& alloc, arctic::String alias) noexcept
        : alias{ alias }
        , _allocator{ alloc }
        , _usertypes{ alloc._backing }
        , _functions{ alloc._backing }
        , _native_functions{ alloc._backing }
        , _variables{ alloc._backing }
    {
    }

    // auto ASLScriptFile::find_alias(arctic::SyntaxNode<> node_annotation) noexcept -> arctic::String
    // {
    //     IPT_ZONE_SCOPED;

    //     arctic::String type_alias;
    //     for (auto annotation : detail::arc_foreach<arctic::syntax::AnnotationAttrib>(node_annotation))
    //     {
    //         if (annotation.data().name == "glsl:type")
    //         {
    //             arctic::String value = annotation.data().value;
    //             if (value.starts_with('"'))
    //             {
    //                 size_t const start_pos = value.find_first_not_of('"');
    //                 size_t const end_pos = value.find_last_not_of('"');
    //                 value = value.substr(start_pos, (end_pos - start_pos) + 1);
    //             }

    //             ICE_ASSERT_CORE(type_alias.empty());
    //             type_alias = value;
    //         }
    //     }
    //     return type_alias;
    // }

    auto ASLScriptFile::find(arctic::String identifier) noexcept -> arctic::SyntaxNode<>
    {
        IPT_ZONE_SCOPED;

        ice::u64 const hash_identifier = detail::arc_hash(identifier);

        arctic::SyntaxNode<> result = ice::hashmap::get(_usertypes, hash_identifier, arctic::SyntaxNode<arctic::syntax::Struct>{});
        if (result == false)
        {
            result = ice::hashmap::get(_functions, hash_identifier, arctic::SyntaxNode<arctic::syntax::Function>{});
        }
        if (result == false)
        {
            result = ice::hashmap::get(_native_functions, hash_identifier, arctic::SyntaxNode<arctic::syntax::Function>{});
        }
        if (result == false)
        {
            result = ice::hashmap::get(_variables, hash_identifier, arctic::SyntaxNode<arctic::syntax::ContextVariable>{});
        }
        return result;
    }

    void ASLScriptFile::visit(arctic::SyntaxNode<arctic::syntax::Define> node) noexcept
    {
        IPT_ZONE_SCOPED;

        arctic::SyntaxNode<arctic::syntax::Struct> usertype{ _allocator };
        usertype.data().name = node.data().name;

        if (node.data().is_native)
        {
            arctic::SyntaxNode<arctic::syntax::Type> native_type{ _allocator };
            native_type.data().is_mutable = false;
            native_type.data().name = node.data().alias;

            usertype.append_annotation(ice::move(native_type));
        }

        // Store the user type node
        ice::multi_hashmap::insert(_usertypes, detail::arc_hash(node.data().name.value), ice::move(usertype));
    }

    void ASLScriptFile::visit(arctic::SyntaxNode<arctic::syntax::Struct> node) noexcept
    {
        IPT_ZONE_SCOPED;

        // Check for compatible annotations...
        arctic::String const name = node.data().name.value;

        // Create a native type node to be attached instead of annotations.
        arctic::SyntaxNode<arctic::syntax::Type> native_type{ _allocator };
        native_type.data().is_mutable = false;
        native_type.data().name = node.data().name; // Set original type as default

        // We replace the annotations with custom nodes.
        node.replace_annotation(ice::move(native_type));

        // Store the user type node
        ice::multi_hashmap::insert(_usertypes, detail::arc_hash(name), node);
    }

    void ASLScriptFile::visit(arctic::SyntaxNode<arctic::syntax::Function> node) noexcept
    {
        IPT_ZONE_SCOPED;

        // We ignore all non-native functions
        if (node.data().is_natvie == false)
        {
            ice::multi_hashmap::insert(_native_functions, detail::arc_hash(node.data().name.value), node);
        }
        else
        {
            ice::multi_hashmap::insert(_functions, detail::arc_hash(node.data().name.value), node);
        }
    }

    void ASLScriptFile::visit(arctic::SyntaxNode<arctic::syntax::ContextBlock> node) noexcept
    {
    }

} // namespace ice
