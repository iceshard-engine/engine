#include "shader_tools_wgsl_patcher.hxx"

namespace ice
{

    //! \details Transform "sampleTexture(sampl, text, coord)" to "texture(sampler2D(text, sampl), coord)"
    void wgsl_native_sampleTexture(
        arctic::SyntaxNode<arctic::syntax::Call> node,
        arctic::SyntaxNodeAllocator& alloc
    ) noexcept
    {
        IPT_ZONE_SCOPED;
        arctic::Token fn = node.data().name;

        arctic::SyntaxNode<> sampler_arg = node.replace_child(arctic::SyntaxNode<>{});
        arctic::SyntaxNode<> texture_arg = sampler_arg.replace_sibling(arctic::SyntaxNode<>{});
        arctic::SyntaxNode<> uv_arg = texture_arg.replace_sibling(arctic::SyntaxNode<>{});

        node.set(&arctic::syntax::Call::name, arctic::Token{
            .value = "textureSample",
            .type = arctic::TokenType::CT_Symbol,
            .location = fn.location
            });

        node.append_child(texture_arg);
        node.append_child(sampler_arg);
        node.append_child(uv_arg);
    }

    auto wgsl_find_alias(arctic::SyntaxNode<> node_annotation) noexcept -> arctic::String
    {
        IPT_ZONE_SCOPED;

        arctic::String type_alias;
        for (auto annotation : detail::arc_foreach<arctic::syntax::AnnotationAttrib>(node_annotation))
        {
            if (annotation.data().name == "wgsl:type")
            {
                arctic::String value = annotation.data().value;
                if (value.starts_with('"'))
                {
                    size_t const start_pos = value.find_first_not_of('"');
                    size_t const end_pos = value.find_last_not_of('"');
                    value = value.substr(start_pos, (end_pos - start_pos) + 1);
                }

                ICE_ASSERT_CORE(type_alias.empty());
                type_alias = value;
            }
        }
        return type_alias;
    }

    void WGSLPatcher::visit(arctic::SyntaxNode<arctic::syntax::Define> node) noexcept
    {
        if (node.data().is_native == false)
        {
            return ASLPatcher::visit(node);
        }

        // If compatible annotations where found replace them.
        if (arctic::String type_alias = wgsl_find_alias(node.annotation()); type_alias.empty() == false)
        {
            // Set the name token
            arctic::Token const alias{
                .value = type_alias,
                .type = arctic::TokenType::CT_Symbol,
                // Use the same locations like the struct
                .location = node.data().name.location
            };

            node.set(&arctic::syntax::Define::alias, alias);
        }
        else
        {
            ASLPatcher::visit(node);
        }
    }

    void WGSLPatcher::visit(arctic::SyntaxNode<arctic::syntax::Call> node) noexcept
    {
        arctic::String const name = node.data().name.value;
        arctic::SyntaxNode<arctic::syntax::Function> func = _imports.find(node.data().name.value).to<arctic::syntax::Function>();
        if (func && func.data().is_natvie)
        {
            if (name == "sampleTexture")
            {
                wgsl_native_sampleTexture(node, _allocator);
            }
            else
            {
                return ASLPatcher::visit(node);
            }
        }
        else
        {
            return ASLPatcher::visit(node);
        }
    }

} // namespace ice
