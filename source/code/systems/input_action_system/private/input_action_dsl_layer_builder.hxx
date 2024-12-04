#pragma once
#include "input_action_dsl.hxx"
#include <ice/input_action_layer_builder.hxx>

namespace ice
{

    class InputActionDSLLayerBuilder : public ice::ActionInputParserEvents
    {
    public:
        InputActionDSLLayerBuilder(
            ice::UniquePtr<ice::InputActionLayerBuilder> builder
        ) noexcept;

        void visit(arctic::SyntaxNode<ice::syntax::Layer> node) noexcept override;

        void visit(arctic::SyntaxNode<ice::syntax::LayerSource> node) noexcept;

        void visit(arctic::SyntaxNode<ice::syntax::LayerAction> node) noexcept;

        void visit(
            ice::InputActionLayerBuilder::ActionBuilder& action,
            arctic::SyntaxNode<ice::syntax::LayerActionWhen> node
        ) noexcept;

        void visit(
            ice::InputActionLayerBuilder::ActionBuilder& action,
            arctic::SyntaxNode<ice::syntax::LayerActionStep> node
        ) noexcept;

        auto finalize(ice::Allocator& alloc) noexcept { return _builder->finalize(alloc); }

    private:
        ice::UniquePtr<ice::InputActionLayerBuilder> _builder;
    };


} // namespace ice
