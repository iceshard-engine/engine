#include "shader_tools_asl_patcher.hxx"
#include <ice/profiler.hxx>

namespace ice
{

    ASLPatcher::ASLPatcher(ASLAllocator& alloc, ASLImportTracker& imports) noexcept
        : _allocator{ alloc }
        , _imports{ imports }
    { }

    void ASLPatcher::visit(arctic::SyntaxNode<> node) noexcept
    {
        ice::ASLPatcherVisitors::visit(node);
    }

    void ASLPatcher::visit(arctic::SyntaxNode<arctic::syntax::Define> node) noexcept
    {
    }

    void ASLPatcher::visit(arctic::SyntaxNode<arctic::syntax::Type> node) noexcept
    {
        IPT_ZONE_SCOPED;

        arctic::SyntaxNode<> alias_node = _imports.find(node.data().name.value);
        arctic::SyntaxNode<arctic::syntax::Type> alias_type = alias_node.annotation().to<arctic::syntax::Type>();
        if (alias_type)
        {
            node.data().name = alias_type.data().name;
        }
    }

    void ASLPatcher::visit(arctic::SyntaxNode<arctic::syntax::Call> node) noexcept
    {
        IPT_ZONE_SCOPED;

        arctic::SyntaxNode<> alias_node = _imports.find(node.data().name.value);
        arctic::SyntaxNode<arctic::syntax::Type> alias_type = alias_node.annotation().to<arctic::syntax::Type>();
        if (alias_type)
        {
            node.data().name = alias_type.data().name;
        }
    }

} // namespace ice
