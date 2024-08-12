#include "shader_tools_glsl.hxx"

#if ISP_WINDOWS
#include <ice/task_expected.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/container/array.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/string_utils.hxx>
#include <ice/path_utils.hxx>
#include <ice/profiler.hxx>

#include "shader_tools_asl.hxx"
#include "shader_tools_asl_allocator.hxx"
#include "shader_tools_asl_database.hxx"
#include "shader_tools_asl_importer.hxx"
#include "shader_tools_asl_script.hxx"
#include "shader_tools_asl_shader.hxx"
#include "shader_tools_glsl_patcher.hxx"

#include <shaderc/shaderc.hpp>

namespace ice
{

    namespace glsl
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

            arctic::String const fn_name = func.name.value;
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
            ice::string::push_format(result, "#version 450\n\n");

            // Generate struct definitions
            for (SyntaxNode<syntax::Struct> strct : shader._structs)
            {
                if (strct == false)
                {
                    continue;
                }

                // Don't generate structs for known uniform variables.
                bool found = false;
                for (SyntaxNode<syntax::ContextVariable> variable : shader._uniforms)
                {
                    found |= variable.child<syntax::Type>().data().name.value == strct.data().name.value;
                }
                if (found)
                {
                    continue;
                }

                ice::string::push_format(result, "struct {} {{\n", strct.data().name.value);
                SyntaxNode<syntax::StructMember> member = strct.child<syntax::StructMember>();
                while (member)
                {
                    syntax::Type const& type = member.child<syntax::Type>().data();
                    ice::string::push_format(result, "    {} {};\n", type.name.value, member.data().name.value);
                    member = member.sibling<syntax::StructMember>();
                }
                ice::string::push_back(result, "};\n\n");
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

            // Generate shader inputs and outputs
            SyntaxNode<syntax::StructMember> member = shader._inputs.child<syntax::StructMember>();
            while (member)
            {
                syntax::Type const& type = member.child<syntax::Type>().data();
                arctic::String location;
                if (detail::arc_annotation(member, "location", location))
                {
                    ice::string::push_format(
                        result, "layout (location={}) in {} in_{};\n",
                        location, type.name.value, /*func_arg_name, */ member.data().name.value
                    );
                }
                member = member.sibling<syntax::StructMember>();
            }

            ice::string::push_back(result, "\n");

            member = shader._outputs.child<syntax::StructMember>();
            while (member)
            {
                syntax::Type const& type = member.child<syntax::Type>().data();
                arctic::String location;
                if (detail::arc_annotation(member, "location", location))
                {
                    ice::string::push_format(
                        result, "layout (location={}) out {} out_{};\n",
                        location, type.name.value, /*func_arg_name, */ member.data().name.value
                    );
                }
                member = member.sibling<syntax::StructMember>();
            }

            ice::string::push_back(result, "\n");

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
                    ice::string::push_format(result, "layout (std140, set={}, binding={}) uniform {} {{\n",
                        set, binding, variable.child<syntax::Type>().data().name.value
                    );

                    while (member)
                    {
                        syntax::Type const& type = member.child<syntax::Type>().data();
                        ice::string::push_format(result, "    {} {};\n", type.name.value, member.data().name.value);
                        member = member.sibling<syntax::StructMember>();
                    }

                    ice::string::push_format(result, "}} {};\n\n", variable.data().name.value);
                }
                else
                {
                    arctic::String const type = variable.child<syntax::Type>().data().name.value;
                    arctic::String const name = variable.data().name.value;
                    ice::string::push_format(result, "layout(set={}, binding={}) uniform {} {};\n", set, binding, type, name);
                }
            }

            if (shader._pushcontants)
            {
                SyntaxNode<syntax::Struct> strct = shader.find_struct(shader._pushcontants.child<syntax::Type>());

                ice::string::push_format(result, "layout(push_constant) uniform {} {{\n",
                    shader._pushcontants.child<syntax::Type>().data().name.value
                );
                member = strct.child<syntax::StructMember>();
                while (member)
                {
                    syntax::Type const& type = member.child<syntax::Type>().data();
                    ice::string::push_format(result, "    {} {};\n", type.name.value, member.data().name.value);
                    member = member.sibling<syntax::StructMember>();
                }
                ice::string::push_format(result, "}} {};\n\n", shader._pushcontants.data().name.value);
            }

            // Generate shader main
            SyntaxNode<syntax::FunctionArg> arg = shader._mainfunc.child<syntax::FunctionArg>();
            SyntaxNode<syntax::Type> ret = arg.sibling<syntax::Type>();
            SyntaxNode<syntax::FunctionBody> body = ret.sibling<syntax::FunctionBody>();
            ICE_ASSERT_CORE(ret && body);


            ice::string::push_format(result, "\nvoid asl_proxy_{}(", shader._mainfunc.data().name.value);
            ice::hashmap::set(subs, detail::arc_hash(arg.data().name.value), arctic::String{ "_a_inputs" });
            ice::hashmap::set(subs, detail::arc_hash(shader._mainfunc.data().name.value), arctic::String{ "_a_outputs" });
            ice::string::push_format(result, "in {} _a_inputs, ", shader._inputs.data().name.value, arg.data().name.value);
            ice::string::push_format(result, "out {} _a_outputs) {{\n", shader._outputs.data().name.value, shader._mainfunc.data().name.value);
            generate_function(result, subs, shader._mainfunc.data(), arg.data(), ret.data(), body.child<syntax::Expression>());
            ice::string::push_back(result, "}\n");

            ice::string::push_format(result, "\nvoid {}() {{\n", "main"); // GLSL requires 'main' as the function name
            ice::string::push_format(result, "    {0} inputs = {0}(", shader._inputs.data().name.value);
            {
                member = shader._inputs.child<syntax::StructMember>();
                while (member)
                {
                    arctic::String builtin;
                    if (detail::arc_annotation(member, "builtin", builtin))
                    {
                        if (builtin == "position")
                        {
                            ice::string::push_back(result, "gl_FragCoord, ");
                        }
                    }
                    else
                    {
                        ice::string::push_format(result, "in_{}, ", member.data().name.value);
                    }
                    member = member.sibling<syntax::StructMember>();
                }
                ice::string::pop_back(result, 2);
            }
            ice::string::push_back(result, ");\n");
            ice::string::push_format(result, "    {} outputs;\n", shader._outputs.data().name.value);
            ice::string::push_format(result, "    asl_proxy_{}(inputs, outputs);\n", shader._mainfunc.data().name.value);
            {
                member = shader._outputs.child<syntax::StructMember>();
                while (member)
                {
                    arctic::String builtin;
                    if (detail::arc_annotation(member, "builtin", builtin))
                    {
                        if (builtin == "position")
                        {
                            ice::string::push_format(result, "    gl_Position = outputs.{};\n", member.data().name.value);
                        }
                    }
                    else
                    {
                        ice::string::push_format(result, "    out_{0} = outputs.{0};\n", member.data().name.value);
                    }
                    member = member.sibling<syntax::StructMember>();
                }
            }
            ice::string::push_back(result, "}\n");

            return result;
        };

        auto transpile_shader_asl_to_glsl(
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

            GLSLPatcher glsl_patcher{ asl_alloc, asl_imports };
            asl_imports.add_visitor(&glsl_patcher);

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

            arctic::SyntaxVisitor* visitors[]{ &glsl_patcher, &asl_shaderfile, &asl_imports, &asl_shader };

            ice::HeapString<> str{ alloc };
            if (parser->parse(lexer, asl_alloc, visitors))
            {
                str = generate(alloc, asl_shader);
                out_entry_point = "main";
            }

            arctic::shutdown_matcher(&matcher);
            return str;
        }

        bool collect_shader_sources(
            ice::ResourceHandle* resource_handle,
            ice::ResourceTracker& resource_tracker,
            ice::Array<ice::ResourceHandle*>& out_sources
        ) noexcept
        {
            return false;
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

                out_result = transpile_shader_asl_to_glsl(
                    alloc,
                    *import_loader,
                    result.data,
                    { shader_stage },
                    out_entry_point
                );

                // Failed to transpile
                if (ice::string::empty(out_result))
                {
                    co_return E_FailedToTranspileASLShaderToGLSL;
                }
                co_return out_result;
            }
            else
            {
                co_return ice::String{
                    (char const*)result.data.location,
                    (ice::ucount)result.data.size.value
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
            ice::Expected<ice::String, ice::ErrorCode> result = co_await glsl::load_shader_source(
                alloc, tracker, source, shader_stage, transpiled_result, entry_point
            );

            if (result.valid() == false)
            {
                ICE_LOG(LogSeverity::Error, LogTag::System, "Failed to load shader '{}' with error '{}'", path, result.error());
                co_return{ };
            }

            ice::Memory result_mem{};
            if (sctx.stage == ShaderStage::Compiled)
            {
                ice::String const glsl_source = result.value();

                shaderc::CompileOptions compile_options{};

                // We don't use optimization in runtime-baked shaders
                compile_options.SetOptimizationLevel(shaderc_optimization_level_zero);
                compile_options.SetTargetSpirv(shaderc_spirv_version_1_6); // TODO: take from the metadata / platform settings?
                shaderc::Compiler compiler{};

                shaderc::SpvCompilationResult const spv_result = compiler.CompileGlslToSpv(
                    ice::string::begin(glsl_source),
                    ice::string::size(glsl_source),
                    is_vertex_shader ? shaderc_shader_kind::shaderc_vertex_shader : shaderc_shader_kind::shaderc_fragment_shader,
                    ice::string::begin(path),
                    ice::string::begin(entry_point),
                    compile_options
                );

                // Check if we were successful
                if (spv_result.GetCompilationStatus() != shaderc_compilation_status_success)
                {
                    ICE_LOG(LogSeverity::Error, LogTag::System, "Failed to load shader '{}' with error '{}'", path, spv_result.GetErrorMessage());
                }
                else
                {
                    // TODO List warnings

                    // Spv result is a 4byte BC table
                    ice::usize const result_size = ice::size_of<ice::u32> *(spv_result.end() - spv_result.begin());
                    result_mem = alloc.allocate(result_size);
                    ice::memcpy(result_mem.location, spv_result.begin(), result_size);
                }
            }
            else
            {
                // Keep the string size so we can adjust the memory block result
                ice::ucount const string_size = ice::size(transpiled_result);
                result_mem = ice::string::extract_memory(transpiled_result);

                // Ensure memory size is equal to string size not its capacity.
                result_mem.size.value = string_size;
            }

            // Unload resource before continuing
            co_await tracker.unload_resource(source);

            // Check if we were successful
            if (result_mem.location == nullptr)
            {
                co_return{ };
            }

            // TODO:
            sctx.shader_main = entry_point;
            sctx.shader_type = static_cast<ice::i32>(shader_stage);

            // Move the memory from the heapstring to Memory
            co_return ResourceCompilerResult{ .result = result_mem };
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

    } // namespace glsl

} // namespace ice

#endif // #if ISP_WINDOWS
