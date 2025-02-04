/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "shader_tools_asl_importer.hxx"

#include <ice/resource_tracker.hxx>
#include <ice/string_utils.hxx>
#include <ice/task_utils.hxx>
#include <ice/profiler.hxx>
#include <ice/uri.hxx>

namespace ice
{

    class ASLResourceScriptLoader : public ASLScriptLoader
    {
    public:
        ASLResourceScriptLoader(
            ice::Allocator& alloc,
            ice::ResourceTracker& tracker
        ) noexcept
            : _allocator{ alloc }
            , _tracker{ tracker }
        { }

        auto load_source(arctic::String import_path) noexcept -> arctic::String override
        {
            IPT_ZONE_SCOPED;

            // Imports don't specify the extension so we need to add it.
            ice::HeapString<> import_path_final{ _allocator };
            ice::string::push_format(import_path_final, "{}.asl", import_path);

            // Find the resource to he loaded.
            ice::ResourceHandle const import_resource = _tracker.find_resource(
                ice::URI{ ice::Scheme_URN, { ice::String{ import_path_final } } }
            );
            if (import_resource == nullptr)
            {
                ICE_LOG(LogSeverity::Error, LogTag::Tool, "Failed to import ASL file: {}", ice::String{ import_path_final });
                return {};
            }

            // Try to load the mentioned import script
            ice::ResourceResult res = ice::wait_for_result(_tracker.load_resource(import_resource));
            if (res.resource_status != ResourceStatus::Loaded)
            {
                return {};
            }

            // Return the script contents
            return arctic::String{ (char const*)res.data.location, res.data.size.value };
        }

    private:
        ice::Allocator& _allocator;
        ice::ResourceTracker& _tracker;
    };

    auto create_script_loader(
        ice::Allocator& alloc,
        ice::ResourceTracker& tracker
    ) noexcept -> ice::UniquePtr<ice::ASLScriptLoader>
    {
        return ice::make_unique<ice::ASLResourceScriptLoader>(alloc, alloc, tracker);
    }

    auto parse_import_file(
        ice::ASLAllocator& alloc,
        ice::ASLImportTracker& imports,
        ice::Span<arctic::SyntaxVisitor*> visitors,
        arctic::String asl_source,
        arctic::String asl_alias
    ) noexcept -> ice::UniquePtr<ASLScriptFile>;

    ASLImportTracker::ASLImportTracker(
        ice::ASLAllocator& alloc,
        ice::ASLScriptLoader& resolver
    ) noexcept
        : _allocator{ alloc }
        , _resolver{ resolver }
        , _script_visitors{ _allocator._backing }
        , _imports{ _allocator._backing }
        , _aliases{ _allocator._backing }
        , _global{ _allocator._backing }
    {
    }

    void ASLImportTracker::track_script(ASLScriptFile* file) noexcept
    {
        // Store the tracker pointer in a list.
        ice::array::push_back(_global, file);
    }

    void ASLImportTracker::add_visitor(arctic::SyntaxVisitor* visitor) noexcept
    {
        ice::array::push_back(_script_visitors, visitor);
    }

    auto ASLImportTracker::find(arctic::String identifier) noexcept -> arctic::SyntaxNode<>
    {
        IPT_ZONE_SCOPED;

        size_t const pos = identifier.find_first_of('.');
        if (pos == std::string_view::npos)
        {
            for (ASLEntityTracker* tracker : _global)
            {
                // Find and return type first entity matching this identifier
                if (arctic::SyntaxNode<> result = tracker->find(identifier); result)
                {
                    return result;
                }
            }
        }
        else
        {
            arctic::String const potential_alias_key = identifier.substr(0, pos);
            ice::u64 const hash_alias_key = detail::arc_hash(potential_alias_key);

            ASLScriptFile* const file = ice::hashmap::get(_aliases, hash_alias_key, nullptr);
            ICE_ASSERT_CORE(file == nullptr || file->alias == potential_alias_key);
            if (file != nullptr)
            {
                return file->find(identifier.substr(pos + 1));
            }
        }
        return {};
    }

    void ASLImportTracker::visit(arctic::SyntaxNode<> node) noexcept
    {
        arctic::SyntaxVisitorGroup<arctic::syntax::Import>::visit(node);
    }

    void ASLImportTracker::visit(arctic::SyntaxNode<arctic::syntax::Import> node) noexcept
    {
        IPT_ZONE_SCOPED;

        arctic::String const import_path = node.data().path;
        arctic::String const import_alias = node.data().alias;
        ice::u64 const hash_import_path = detail::arc_hash(import_path);
        ice::u64 const hash_import_alias = detail::arc_hash(import_alias);

        Entry* const entry = ice::hashmap::try_get(_imports, hash_import_path);
        ASLEntityTracker* const aliased = ice::hashmap::get(_aliases, hash_import_alias, nullptr);

        // The alias is either the same as the entry, or there is no alias
        ICE_ASSERT_CORE(entry == nullptr || entry->file.get() == aliased || aliased == nullptr);
        if (entry != nullptr)
        {
            // Safe the new alias if one was provided
            if (aliased == nullptr && import_alias.empty() == false)
            {
                ice::hashmap::set(_aliases, hash_import_alias, entry->file.get());
            }
            return;
        }

        // Try to access the source
        arctic::String source = _resolver.load_source(import_path);
        if (source.empty())
        {
            return;
        }

        // Create a new entry since it's the first time we see this import
        Entry import_entry{
            .path = import_path,
            .file = parse_import_file(_allocator, *this, _script_visitors, source, import_alias)
        };
        if (import_entry.file == nullptr)
        {
            return;
        }

        // Store the tracker pointer in a list.
        ice::array::push_back(_global, import_entry.file.get());

        // Store the whole entry.
        ice::multi_hashmap::insert(_imports, detail::arc_hash(node.data().path), ice::move(import_entry));
    }

    auto parse_import_file(
        ice::ASLAllocator& alloc,
        ice::ASLImportTracker& imports,
        ice::Span<arctic::SyntaxVisitor*> visitors,
        arctic::String asl_source,
        arctic::String asl_alias
    ) noexcept -> ice::UniquePtr<ASLScriptFile>
    {
        IPT_ZONE_SCOPED;

        arctic::WordMatcher matcher{ };
        arctic::initialize_default_matcher(&matcher);

        ice::UniquePtr<ice::ASLScriptFile> result;
        {
            // Create a lexer for the given input source
            arctic::Lexer lexer = arctic::create_lexer(
                arctic::create_word_processor(asl_source, &matcher)
            );

            // Initiaize the parser
            arctic::ParserCreateInfo const parser_info{ .rules = arctic::grammar::Constant_GlobalRules };
            std::unique_ptr<arctic::Parser> parser = arctic::create_default_parser(parser_info);

            // Prepare visitors
            result = ice::make_unique<ASLScriptFile>(alloc._backing, alloc, asl_alias);

            ice::Array<arctic::SyntaxVisitor*> final_visitors{ alloc._backing, visitors };
            ice::array::push_back(final_visitors, &imports);
            ice::array::push_back(final_visitors, result.get());

            if (parser->parse(lexer, alloc, final_visitors) == false)
            {
                // Release the result if we failed to parse the script
                result = nullptr;
            }
        }

        arctic::shutdown_matcher(&matcher);
        return result;
    }

} // namespace ice
