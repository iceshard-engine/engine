#include "shader_tools_wgsl.hxx"

#if ISP_WEBAPP || ISP_WINDOWS
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
                arctic::String const atom_sub = op.sibling<syntax::Atom>().data().value.value;
                var_base = ice::hashmap::get(subs, detail::arc_hash(var_base), var_base);

                ice::string::push_format(out_code, "{}.{}", var_base, atom_sub);
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
            syntax::Type const& ret,
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
                        ice::string::push_back(result, "(");
                        generate_expression(result, subs, func, arg, ret, node.child());
                        ice::string::push_back(result, ")");
                    }
                    else
                    {
                        generate_atom(result, subs, func, arg, node.to<syntax::Atom>());
                    }
                    break;
                }
                case SyntaxEntity::E_CallArg:
                {
                    generate_expression(result, subs, func, arg, ret, node.child());
                    if (node.sibling())
                    {
                        ice::string::push_back(result, ", ");
                    }
                    break;
                }
                case SyntaxEntity::E_Operator:
                {
                    syntax::Operator const& op = node.to<syntax::Operator>().data();
                    ice::string::push_format(result, " {} ", op.token.value);
                    if (node.child())
                    {
                        generate_expression(result, subs, func, arg, ret, node.child());
                    }
                    break;
                }
                case SyntaxEntity::E_Call:
                    ice::string::push_format(result, "{}(", node.to<syntax::Call>().data().name.value);
                    // Call children groups
                    generate_expression(result, subs, func, arg, ret, node.child());
                    ice::string::push_back(result, ")");
                    break;
                default:
                    break;
                }

                node = node.sibling();
            }
        }

        auto generate_function(
            ice::HeapString<>& result,
            ice::HashMap<arctic::String> const& subs,
            syntax::Function const& func,
            syntax::FunctionArg const& arg,
            syntax::Type const& ret,
            SyntaxNode<> exp
        ) noexcept
        {
            IPT_ZONE_SCOPED;

            // arctic::String const fn_name = func.name.value;
            while (exp)
            {
                ice::string::push_back(result, "    ");

                generate_expression(result, subs, func, arg, ret, exp.child());

                ice::string::push_back(result, ";\n");
                exp = exp.sibling<syntax::Expression>();
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
                        ice::string::push_format(
                            result, "    @location({}) {}: {},\n",
                            location, member.data().name.value, type.name.value
                        );
                    }
                    else if (detail::arc_annotation(member, "builtin", location))
                    {
                        is_inout = true;
                        ice::string::push_format(
                            result, "    @builtin({}) {}: {},\n",
                            location, member.data().name.value, type.name.value
                        );
                    }
                    else
                    {
                        ice::string::push_format(
                            result, "    {}: {},\n",
                            member.data().name.value, type.name.value
                        );
                    }

                    //ice::string::push_format(result, "    {} {};\n", type.name.value, member.data().name.value);
                    member = member.sibling<syntax::StructMember>();
                }
                ice::string::push_back(result, "};\n\n");

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

            ice::string::push_back(result, "\n");

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
            generate_function(result, subs, shader._mainfunc.data(), arg.data(), ret.data(), body.child<syntax::Expression>());
            ice::string::push_back(result, "    return out;\n");
            ice::string::push_back(result, "}\n");
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
            ice::ResourceHandle* source,
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
                if (ice::string::empty(out_result))
                {
                    co_return E_FailedToTranspileASLShaderToWGSL;
                }
                co_return out_result;
            }
            else
            {
                co_return ice::String{
                    (char const*) result.data.location,
                    (ice::ucount) result.data.size.value
                };
            }
        }

        auto compiler_compile_shader_source(
            ice::ResourceCompilerCtx& ctx,
            ice::ResourceHandle* source,
            ice::ResourceTracker& tracker,
            ice::Span<ice::ResourceHandle* const>,
            ice::Span<ice::URI const>,
            ice::Allocator& alloc
        ) noexcept -> ice::Task<ice::ResourceCompilerResult>
        {
            ShaderCompilerContext& sctx = *shader_context(ctx);

            ice::String const path = ice::resource_origin(source);
            ice::String const ext = ice::path::extension(path);
            bool const is_vertex_shader = ice::string::substr(path, ice::string::size(path) - (4 + ice::size(ext)), ice::size(ext)) == "vert";

            ice::render::ShaderStageFlags const shader_stage = is_vertex_shader
                ? ice::render::ShaderStageFlags::VertexStage
                : ice::render::ShaderStageFlags::FragmentStage;

            ice::HeapString<> transpiled_result{ alloc };
            ice::HeapString<> entry_point{ alloc };
            ice::Expected<ice::String, ice::ErrorCode> result = co_await wgsl::load_shader_source(
                alloc, tracker, source, shader_stage, transpiled_result, entry_point
            );

            if (result.valid() == false)
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

            ice::ucount const string_size = ice::size(transpiled_result);
            ice::Memory memory = ice::string::extract_memory(transpiled_result);

            // Set the memory size to the final string size. Add '1' if the results has to be "compiled".
            //  The added '1' is there because WebGPU shader loading functions to accept a size, so we need to ensure the loaded
            //  data is '0' terminated. Returning memory with that '0' character ensure it's valid when loaded into memory.
            memory.size.value = string_size + ice::u32(sctx.stage == ice::ShaderStage::Compiled);

            // Move the memory from the heapstring to Memory
            co_return ResourceCompilerResult{ .result = memory };
        }

        auto compiler_build_shader_meta(
            ice::ResourceCompilerCtx& ctx,
            ice::ResourceHandle* source,
            ice::ResourceTracker& tracker,
            ice::Span<ice::ResourceCompilerResult const>,
            ice::Span<ice::URI const>,
            ice::MutableMetadata& out_meta
        ) noexcept -> ice::Task<bool>
        {
            ShaderCompilerContext sctx = *shader_context(ctx);
            ice::meta_set_int32(out_meta, "ice.shader.stage"_sid, sctx.shader_type);
            ice::meta_set_string(out_meta, "ice.shader.entry_point"_sid, sctx.shader_main);
            co_return true;
        }

    } // namespace wgsl

} // namespace ice

#endif // #if ISP_WEBAPP || ISP_WINDOWS
