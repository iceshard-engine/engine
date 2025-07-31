#pragma once
#include "input_action_dsl.hxx"
#include <ice/input_action_layer_builder.hxx>

namespace ice
{

    class InputActionDSLLayerBuilder : public ice::ActionInputParserEvents
    {
    public:
        InputActionDSLLayerBuilder(
            ice::UniquePtr<ice::InputActionBuilder::Layer> builder
        ) noexcept;

        void visit(arctic::SyntaxNode<ice::syntax::Layer> node) noexcept override;

        void visit_source(arctic::SyntaxNode<ice::syntax::LayerSource> node) noexcept;
        void visit_layer(arctic::SyntaxNode<ice::syntax::LayerAction> node) noexcept;

        auto visit_cond(
            ice::InputActionBuilder::ConditionSeries series,
            arctic::SyntaxNode<ice::syntax::LayerActionWhen> node
        ) noexcept -> arctic::SyntaxNode<>;

        void visit_step(
            ice::InputActionBuilder::ConditionSeries& condition_series,
            arctic::SyntaxNode<ice::syntax::LayerActionStep> node
        ) noexcept;

        void visit_mod(
            ice::InputActionBuilder::Action& action,
            arctic::SyntaxNode<ice::syntax::LayerActionModifier> node
        ) noexcept;

        auto finalize(ice::Allocator& alloc) noexcept { return _builder->finalize(alloc); }

    private:
        ice::UniquePtr<ice::InputActionBuilder::Layer> _builder;
    };


} // namespace ice
