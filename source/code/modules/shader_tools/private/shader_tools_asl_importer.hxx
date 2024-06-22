#pragma once
#include "shader_tools_asl.hxx"
#include "shader_tools_asl_allocator.hxx"
#include "shader_tools_asl_database.hxx"
#include "shader_tools_asl_script.hxx"
#include "shader_tools_asl_utils.cxx"

#include <arctic/arctic_types.hxx>
#include <arctic/arctic_syntax.hxx>
#include <ice/resource_types.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/profiler.hxx>

namespace ice
{

    struct ASLScriptLoader
    {
        virtual ~ASLScriptLoader() noexcept = default;

        virtual auto load_source(arctic::String import_path) noexcept -> arctic::String = 0;
    };

    auto create_script_loader(
        ice::Allocator& alloc,
        ice::ResourceTracker& tracker
    ) noexcept -> ice::UniquePtr<ice::ASLScriptLoader>;

    class ASLImportTracker final : public ice::ASLEntityTracker, public arctic::SyntaxVisitorGroup<arctic::syntax::Import>
    {
    public:
        ASLImportTracker(
            ice::ASLAllocator& alloc,
            ice::ASLScriptLoader& resolver
        ) noexcept;

        void track_script(ASLScriptFile* file) noexcept;
        void add_visitor(arctic::SyntaxVisitor* visitor) noexcept;

    public: // Implements: ice::ASLEntityTracker
        auto find(arctic::String identifier) noexcept -> arctic::SyntaxNode<> override;

    public: // Implements: arctic::SyntaxVisitorGroup<arctic::syntax::Import>
        void visit(arctic::SyntaxNode<> node) noexcept override;
        void visit(arctic::SyntaxNode<arctic::syntax::Import> node) noexcept override;

    private:
        struct Entry
        {
            arctic::String path;
            ice::UniquePtr<ASLScriptFile> file;
        };

        ice::ASLAllocator& _allocator;
        ice::ASLScriptLoader& _resolver;
        ice::Array<arctic::SyntaxVisitor*> _script_visitors;

        ice::HashMap<Entry> _imports;
        ice::HashMap<ASLScriptFile*> _aliases;
        ice::Array<ASLScriptFile*> _global;
    };

} // namespace ice
