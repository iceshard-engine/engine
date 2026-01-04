/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "shader_tools_wgsl.hxx"

#if ISP_WINDOWS || ISP_LINUX || ISP_WEBAPP
#include <ice/task_expected.hxx>
#include <ice/render/render_shader.hxx>
#include <ice/string/string.hxx>
#include <ice/string_utils.hxx>
#include <ice/path_utils.hxx>
#include <ice/log.hxx>

#include "shader_tools_asl.hxx"
#include "shader_tools_asl_allocator.hxx"
#include "shader_tools_asl_database.hxx"
#include "shader_tools_asl_importer.hxx"
#include "shader_tools_asl_script.hxx"
#include "shader_tools_asl_shader.hxx"
#include "shader_tools_wgsl_patcher.hxx"

namespace ice
{

    namespace wgsl
    {

        using namespace arctic;

        void generate_type(
            ice::HeapString<>& out_code,
            arctic::String varname,
            arctic::syntax::Type const& type
        ) noexcept
        {
            if (type.is_array)
            {
                ice::string::push_format(
                    out_code,
                    "{}: array<{}, {}>,\n",
                    varname,
                    type.name.value,
                    type.size_array.value
                );
            }
            else
            {
                ice::string::push_format(
                    out_code,
                    "{}: {},\n",
                    varname,
                    type.name.value

                );
            }
        }

        void generate_expression(
            ice::HeapString<>& result,
            ice::HashMap<arctic::String> const& subs,
            syntax::Function const& func,
            syntax::FunctionArg const& arg,
            SyntaxNode<> node
        ) noexcept;

        void generate_atom(
            ice::HeapString<>& out_code,
            ice::HashMap<arctic::String> const& subs,
            syntax::Function const& func,
            syntax::FunctionArg const& arg,
            SyntaxNode<syntax::Atom> atom
        ) noexcept
        {
            IPT_ZONE_SCOPED;

            SyntaxNode<syntax::Operator> op = atom.child<syntax::Operator>();
            if (op && op.data().token.type == TokenType::CT_Dot)
            {
                arctic::String var_base = atom.data().value.value;
                var_base = ice::hashmap::get(subs, detail::arc_hash(var_base), var_base);

                ice::string::push_format(out_code, "{}.", var_base);
                generate_expression(out_code, subs, func, arg, op.sibling());
            }
            else
            {
                arctic::String const atom_sub = atom.data().value.value;
                ice::string::push_format(out_code, "{}", atom_sub);
            }
        }

        void generate_expression(
            ice::HeapString<>& result,
            ice::HashMap<arctic::String> const& subs,
            syntax::Function const& func,
            syntax::FunctionArg const& arg,
            SyntaxNode<> node
        ) noexcept
        {
            IPT_ZONE_SCOPED;

            while (node)
            {
                switch (node.type())
                {
                case SyntaxEntity::E_Atom:
                {
                    syntax::Atom const& atom = node.to<syntax::Atom>().data();
                    if (atom.is_parenthized)
                    {
                        result.push_back("(");
                        generate_expression(result, subs, func, arg, node.child());
                        result.push_back(")");
                    }
                    else
                    {
                        generate_atom(result, subs, func, arg, node.to<syntax::Atom>());
                    }
                    break;
                }
                case SyntaxEntity::E_CallArg:
                {
                    generate_expression(result, subs, func, arg, node.child());
                    if (node.sibling())
                    {
                        result.push_back(", ");
                    }
                    break;
                }
                case SyntaxEntity::E_Operator:
                {
                    syntax::Operator const& op = node.to<syntax::Operator>().data();
                    if (op.is_unary)
                    {
                        ice::string::push_format(result, "{}", op.token.value);
                    }
                    else
                    {
                        ice::string::push_format(result, " {} ", op.token.value);
                    }

                    if (node.child())
                    {
                        generate_expression(result, subs, func, arg, node.child());
                    }
                    break;
                }
                case SyntaxEntity::E_Call:
                    ice::string::push_format(result, "{}(", node.to<syntax::Call>().data().name.value);
                    // Call children groups
                    generate_expression(result, subs, func, arg, node.child());
                    result.push_back(")");
                    break;
                case SyntaxEntity::E_IndexOperator:
                    result.push_back("[");
                    generate_expression(result, subs, func, arg, node.child());
                    result.push_back("]");
                    break;
                default:
                    break;
                }

                node = node.sibling();
            }
        }

        void generate_variable(
            ice::HeapString<>& result,
            ice::HashMap<arctic::String> const& subs,
            syntax::Function const& func,
            syntax::FunctionArg const& arg,
            syntax::Type const& ret,
            SyntaxNode<syntax::Variable> varnode
        ) noexcept
        {
            IPT_ZONE_SCOPED;

            syntax::Variable const& var = varnode.data();
            SyntaxNode typenode = varnode.child<syntax::Type>();
            ICE_ASSERT_CORE(typenode);

            syntax::Type const& type = typenode.data();
            ice::string::push_format(result, "var {}: {}", var.name.value, type.name.value);

            if (SyntaxNode assignnode = typenode.sibling<syntax::Operator>(); assignnode)
            {
                result.push_back(" = ");

                generate_expression(result, subs, func, arg, assignnode.child());
            }
        }

        auto generate_function(
            ice::HeapString<>& result,
            ice::HashMap<arctic::String> const& subs,
            syntax::Function const& func,
            syntax::FunctionArg const& arg,
            syntax::Type const& ret,
            SyntaxNode<> fnentry
        ) noexcept
        {
            IPT_ZONE_SCOPED;

            while (fnentry)
            {
                result.push_back("    ");

                if (SyntaxNode var = fnentry.to<syntax::Variable>(); var)
                {
                    generate_variable(result, subs, func, arg, ret, var);
                    result.push_back(";\n");
                }
                else if (SyntaxNode exp = fnentry.to<syntax::Expression>(); exp)
                {
                    generate_expression(result, subs, func, arg, exp.child());
                    result.push_back(";\n");
                }
                fnentry = fnentry.sibling<>();
            }
        }

        auto generate(ice::Allocator& alloc, ASLShader& shader) noexcept -> ice::HeapString<>
        {
            IPT_ZONE_SCOPED;

            ice::HeapString<> result{ alloc };

            // Initial lines
            ice::string::push_format(result, "\n");
            //ice::string::push_format(result, "\n/// Generated with IceShard - ShaderTools (target: WGSL)\n\n");

            // Generate struct definitions
            for (SyntaxNode<syntax::Struct> strct : shader._structs)
            {
                if (strct == false)
                {
                    continue;
                }

                bool is_inout = false;
                bool is_uniform = false;
                for (SyntaxNode<syntax::ContextVariable> variable : shader._uniforms)
                {
                    is_uniform |= variable.child<syntax::Type>().data().name.value == strct.data().name.value;
                }

                ice::string::push_format(result, "struct {} {{\n", strct.data().name.value);
                SyntaxNode<syntax::StructMember> member = strct.child<syntax::StructMember>();
                while (member)
                {
                    syntax::Type const& type = member.child<syntax::Type>().data();

                    if (arctic::String location; detail::arc_annotation(member, "location", location))
                    {
                        is_inout = true;
                        ice::string::push_format(result, "    @location({}) ", location);
                    }
                    else if (detail::arc_annotation(member, "builtin", location))
                    {
                        is_inout = true;
                        ice::string::push_format(result, "    @builtin({}) ", location);
                    }
                    else
                    {
                        result.push_back("    ");
                    }

                    wgsl::generate_type(result, member.data().name.value, type);

                    //ice::string::push_format(result, "    {} {};\n", type.name.value, member.data().name.value);
                    member = member.sibling<syntax::StructMember>();
                }
                result.push_back("};\n\n");

                ICE_LOG_IF(
                    is_uniform && is_inout,
                    LogSeverity::Warning, LogTag::Tool,
                    "Struct '{}' used for shader inputs/outputs and uniform variables!",
                    strct.data().name.value
                );
            }

            // Generate function definitions
            for (SyntaxNode<syntax::Function> func : shader._functions)
            {
                if (func == false)
                {
                    continue;
                }
            }

            ice::HashMap<arctic::String> subs{ alloc };
            SyntaxNode<syntax::StructMember> member;

            // Generate uniforms
            for (SyntaxNode<syntax::ContextVariable> variable : shader._uniforms)
            {
                arctic::String set;
                arctic::String binding;
                bool const valid_set = detail::arc_annotation(variable, "group", set);
                bool const valid_binding = detail::arc_annotation(variable, "binding", binding);
                ICE_ASSERT_CORE(valid_set && valid_binding);

                SyntaxNode<syntax::Struct> strct = shader.find_struct(variable.child<syntax::Type>());
                if (member = strct.child<syntax::StructMember>(); member)
                {
                    arctic::String const type = variable.child<syntax::Type>().data().name.value;
                    arctic::String const name = variable.data().name.value;
                    ice::string::push_format(result, "@group({}) @binding({}) var<uniform> {}: {};\n",
                        set, binding, name, type
                    );
                }
                else
                {
                    arctic::String const type = variable.child<syntax::Type>().data().name.value;
                    arctic::String const name = variable.data().name.value;
                    ice::string::push_format(result, "@group({}) @binding({}) var {}: {};\n",
                        set, binding, name, type
                    );
                }
            }

            result.push_back("\n");

            // Generate shader main
            SyntaxNode<syntax::FunctionArg> arg = shader._mainfunc.child<syntax::FunctionArg>();
            SyntaxNode<syntax::Type> ret = arg.sibling<syntax::Type>();
            SyntaxNode<syntax::FunctionBody> body = ret.sibling<syntax::FunctionBody>();
            ICE_ASSERT_CORE(ret && body);

            ice::string::push_format(result, "@{}\n", shader._shader_stage);
            ice::string::push_format(result, "fn {}(", shader._mainfunc.data().name.value);
            ice::string::push_format(
                result,
                "in: {}) -> {} {{\n",
                shader._inputs.data().name.value,
                shader._outputs.data().name.value
            );
            ice::string::push_format(result, "    var out: {};\n", shader._outputs.data().name.value);
            ice::hashmap::set(subs, detail::arc_hash(shader._mainfunc.data().name.value), arctic::String{ "out" });
            generate_function(result, subs, shader._mainfunc.data(), arg.data(), ret.data(), body.child());
            result.push_back("    return out;\n");
            result.push_back("}\n");
            return result;
        };

        auto transpile_shader_asl_to_wgsl(
            ice::Allocator& alloc,
            ice::ASLScriptLoader& resolver,
            ice::Data asl_source,
            ice::ShaderConfig config,
            ice::HeapString<>& out_entry_point
        ) noexcept -> ice::HeapString<>
        {
            IPT_ZONE_SCOPED;

            arctic::String const shader_stage = config.stage == ice::render::ShaderStageFlags::VertexStage
                ? "vertex" : "fragment";

            ASLAllocator asl_alloc{ alloc };
            ASLImportTracker asl_imports{ asl_alloc, resolver };
            ASLScriptFile asl_shaderfile{ asl_alloc, "" };
            ASLShader asl_shader{ alloc, asl_imports, shader_stage };
            asl_imports.track_script(&asl_shaderfile);

            WGSLPatcher wgsl_patcher{ asl_alloc, asl_imports };
            asl_imports.add_visitor(&wgsl_patcher);

            arctic::String const source{
                reinterpret_cast<char const*>(asl_source.location),
                asl_source.size.value
            };

            arctic::WordMatcher matcher{ };
            arctic::initialize_default_matcher(&matcher);

            arctic::Lexer lexer = arctic::create_lexer(
                arctic::create_word_processor(source, &matcher)
            );

            arctic::ParserCreateInfo const parser_info{ .rules = arctic::grammar::Constant_GlobalRules };
            std::unique_ptr<arctic::Parser> parser = arctic::create_default_parser(parser_info);

            arctic::SyntaxVisitor* visitors[]{ &wgsl_patcher, &asl_shaderfile, &asl_imports, &asl_shader };

            ice::HeapString<> str{ alloc };
            if (parser->parse(lexer, asl_alloc, visitors))
            {
                str = generate(alloc, asl_shader);
                out_entry_point = "main";
            }

            arctic::shutdown_matcher(&matcher);
            return str;
        }

        auto load_shader_source(
            ice::Allocator& alloc,
            ice::ResourceTracker& tracker,
            ice::ResourceHandle const& source,
            ice::render::ShaderStageFlags shader_stage,
            ice::HeapString<>& out_result,
            ice::HeapString<>& out_entry_point
        ) noexcept -> ice::TaskExpected<ice::String, ice::ErrorCode>
        {
            ice::ResourceResult const result = co_await tracker.load_resource(source);
            ice::String const path = ice::resource_origin(source);

            if (ice::path::extension(path) == ".asl")
            {
                auto import_loader = ice::create_script_loader(alloc, tracker);

                out_result = transpile_shader_asl_to_wgsl(
                    alloc,
                    *import_loader,
                    result.data,
                    { shader_stage },
                    out_entry_point
                );

                // Failed to transpile
                if (out_result.is_empty())
                {
                    co_return E_FailedToTranspileASLShaderToWGSL;
                }
                co_return out_result;
            }
            else
            {
                co_return ice::String{
                    (char const*) result.data.location,
                    (ice::u32) result.data.size.value
                };
            }
        }

        auto compiler_compile_shader_source(
            ice::ResourceCompilerCtx& ctx,
            ice::ResourceHandle const& source,
            ice::ResourceTracker& tracker,
            ice::Span<ice::ResourceHandle const>,
            ice::Span<ice::URI const>,
            ice::Allocator& alloc
        ) noexcept -> ice::Task<ice::ResourceCompilerResult>
        {
            ShaderCompilerContext& sctx = *shader_context(ctx);

            ice::String const path = ice::resource_origin(source);
            ice::String const ext = ice::path::extension(path);
            bool const is_vertex_shader = path.substr(path.size() - (4 + ext.size()), ext.size()) == "vert";

            ice::render::ShaderStageFlags const shader_stage = is_vertex_shader
                ? ice::render::ShaderStageFlags::VertexStage
                : ice::render::ShaderStageFlags::FragmentStage;

            ice::HeapString<> transpiled_result{ alloc };
            ice::HeapString<> entry_point{ alloc };
            ice::Expected<ice::String, ice::ErrorCode> result = co_await wgsl::load_shader_source(
                alloc, tracker, source, shader_stage, transpiled_result, entry_point
            );

            ICE_LOG(LogSeverity::Debug, LogTag::System, "WGSL: {}", transpiled_result);

            if (result.failed())
            {
                ICE_LOG(LogSeverity::Error, LogTag::System, "Failed to load shader '{}' with error '{}'", path, result.error());
                co_return { };
            }
            ICE_LOG(LogSeverity::Info, LogTag::Tool, "Compiled WebGPU shader resource: {}", ice::resource_path(source));

            // Unload resource before continuing
            co_await tracker.unload_resource(source);

            // TODO:
            sctx.shader_main = entry_point;
            sctx.shader_type = static_cast<ice::i32>(shader_stage);

            ice::ncount const string_size = transpiled_result.size();
            ice::Memory memory = transpiled_result.extract_memory();

            // Set the memory size to the final string size. Add '1' if the results has to be "compiled".
            //  The added '1' is there because WebGPU shader loading functions to accept a size, so we need to ensure the loaded
            //  data is '0' terminated. Returning memory with that '0' character ensure it's valid when loaded into memory.
            memory.size = string_size + ice::u32(sctx.stage == ice::ShaderStage::Compiled);

            // Move the memory from the heapstring to Memory
            co_return ResourceCompilerResult{ .result = memory };
        }

        auto compiler_build_shader_meta(
            ice::ResourceCompilerCtx& ctx,
            ice::ResourceHandle* source,
            ice::ResourceTracker& tracker,
            ice::Span<ice::ResourceCompilerResult const>,
            ice::Span<ice::URI const>,
            ice::ConfigBuilder& out_meta
        ) noexcept -> ice::Task<bool>
        {
            ShaderCompilerContext sctx = *shader_context(ctx);
            out_meta["ice.shader.stage"] = sctx.shader_type;
            out_meta["ice.shader.entry_point"] = ice::String{ sctx.shader_main };
            co_return true;
        }

    } // namespace wgsl

} // namespace ice

#endif // #if ISP_WEBAPP || ISP_WINDOWS
