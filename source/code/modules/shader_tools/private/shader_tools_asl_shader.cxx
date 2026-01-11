/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "shader_tools_asl_shader.hxx"

namespace ice
{

    ASLShader::ASLShader(ice::Allocator& alloc, ASLEntityTracker& tracker, arctic::String shader_stage) noexcept
        : _shader_stage{ shader_stage }
        , _tracker{ tracker }
        , _pushcontants{ }
        , _inputs{ }
        , _outputs{ }
        , _structs{ alloc }
        , _functions{ alloc }
        , _uniforms{ alloc }
    {
    }

    auto ASLShader::find_struct(
        arctic::SyntaxNode<arctic::syntax::Type> const& type
    ) noexcept -> arctic::SyntaxNode<arctic::syntax::Struct>
    {
        return _tracker.find(type.data().name.value).to<arctic::syntax::Struct>();
    }

    void ASLShader::push_struct(arctic::SyntaxNode<arctic::syntax::Struct> node) noexcept
    {
        IPT_ZONE_SCOPED;
        if (node == false)
        {
            return;
        }

        arctic::String annotation;
        if (detail::arc_annotation(node, "shader_stage", annotation) && annotation != _shader_stage)
        {
            return;
        }

        bool already_exists = false;
        for (arctic::SyntaxNode<arctic::syntax::Struct>& strct : _structs)
        {
            if (already_exists = strct.data().name.value == node.data().name.value; already_exists)
            {
                break;
            }
        }

        if (already_exists == false)
        {
            arctic::SyntaxNode<> prev;
            arctic::SyntaxNode<arctic::syntax::StructMember> member = node.child<arctic::syntax::StructMember>();
            while(member)
            {
                if (detail::arc_annotation(member, "shader_stage", annotation) && annotation != _shader_stage)
                {
                    if (prev)
                    {
                        // Remove the member and attach it's sibling to the prev node
                        auto new_prev = member.sibling();
                        prev.replace_sibling(new_prev);
                        prev = new_prev;
                    }
                    else
                    {
                        // Skip this member
                        prev = member.sibling();
                        node.replace_child(prev);
                    }

                    member = member.sibling<arctic::syntax::StructMember>();
                    continue;
                }

                prev = std::exchange(member, member.sibling<arctic::syntax::StructMember>());
            }

            _structs.push_back(ice::move(node));
        }
    }

    void ASLShader::visit(arctic::SyntaxNode<> node) noexcept
    {
        ice::ASLShaderVisitors::visit(node);
    }

    void ASLShader::visit(arctic::SyntaxNode<arctic::syntax::Struct> node) noexcept
    {
        IPT_ZONE_SCOPED;
        push_struct(node);
    }

    void ASLShader::visit(arctic::SyntaxNode<arctic::syntax::Function> node) noexcept
    {
        IPT_ZONE_SCOPED;
        arctic::String annotation;
        if (detail::arc_annotation(node, "shader_stage", annotation) && annotation != _shader_stage)
        {
            return;
        }

        if (detail::arc_annotation(node, "shader_main", annotation))
        {
            arctic::SyntaxNode<arctic::syntax::Type> inputs = node.child<arctic::syntax::FunctionArg>().child<arctic::syntax::Type>();
            arctic::SyntaxNode<arctic::syntax::Type> outputs = node.child().sibling<arctic::syntax::Type>();
            ICE_ASSERT_CORE(inputs == true && outputs == true);

            _inputs = find_struct(inputs);
            _outputs = find_struct(outputs);
            ICE_ASSERT_CORE(_inputs == true && _outputs == true);

            push_struct(_inputs);
            push_struct(_outputs);

            for (arctic::SyntaxNode<arctic::syntax::Struct>& strct : _structs)
            {
                arctic::String const name = strct.data().name.value;
                if (_pushcontants && name == _pushcontants.child<arctic::syntax::Type>().data().name.value)
                {
                    strct = {};
                    continue;
                }
            }

            _mainfunc = node;
            return;
        }
        else
        {
            arctic::SyntaxNode<arctic::syntax::FunctionArg> args = node.child<arctic::syntax::FunctionArg>();
            while(args)
            {
                push_struct(find_struct(args.child<arctic::syntax::Type>()));

                if (auto next = args.sibling<arctic::syntax::FunctionArg>(); next)
                {
                    args = next;
                }
                else if (auto ret = args.sibling<arctic::syntax::Type>())
                {
                    push_struct(find_struct(ret));
                    args = {};
                }
            }
        }

        _functions.push_back(node);
    }

    void ASLShader::visit(arctic::SyntaxNode<arctic::syntax::Variable> node) noexcept
    {
    }

    void ASLShader::visit(arctic::SyntaxNode<arctic::syntax::ContextBlock> node) noexcept
    {
        IPT_ZONE_SCOPED;
        arctic::String annotation;
        if (detail::arc_annotation(node, "shader_stage", annotation) && annotation != _shader_stage)
        {
            return;
        }

        arctic::SyntaxNode<> child = node.child();
        while(child)
        {
            if (auto var = child.to<arctic::syntax::ContextVariable>(); var)
            {
                if (detail::arc_annotation(var, "shader_stage", annotation) && annotation != _shader_stage)
                {
                    child = child.sibling();
                    continue;
                }

                if (detail::arc_annotation(var, "push_constant", annotation))
                {
                    _pushcontants = var;
                    ICE_ASSERT_CORE(_pushcontants == true);
                }
                else if (detail::arc_annotation(var, "uniform", annotation))
                {
                    ICE_ASSERT_CORE(var.data().is_reference == false); // not supported in shaders
                    _uniforms.push_back(var);
                }
                else
                {
                }
            }

            child = child.sibling();
        }
    }

} // namespace ice
