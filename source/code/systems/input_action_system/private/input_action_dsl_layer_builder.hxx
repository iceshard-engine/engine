#pragma once
#include "input_action_script.hxx"
#include <ice/input_action_layer_builder.hxx>

namespace ice
{

    class InputActionDSLLayerBuilder : public ice::asl::ActionInputParserEvents
    {
    public:
        InputActionDSLLayerBuilder(
            ice::UniquePtr<ice::InputActionBuilder::Layer> builder
        ) noexcept;

        void visit(arctic::SyntaxNode<ice::asl::Layer> node) noexcept override;

        void visit_source(arctic::SyntaxNode<ice::asl::LayerSource> node) noexcept;
        void visit_layer(arctic::SyntaxNode<ice::asl::LayerAction> node) noexcept;

        auto visit_cond(
            ice::InputActionBuilder::ConditionSeries series,
            arctic::SyntaxNode<ice::asl::LayerActionCondition> node
        ) noexcept -> arctic::SyntaxNode<>;

        void visit_step(
            ice::InputActionBuilder::ConditionSeries& condition_series,
            arctic::SyntaxNode<ice::asl::LayerActionStep> node
        ) noexcept;

        void visit_mod(
            ice::InputActionBuilder::Action& action,
            arctic::SyntaxNode<ice::asl::LayerActionModifier> node
        ) noexcept;

        auto finalize(ice::Allocator& alloc) noexcept { return _builder->finalize(alloc); }

    private:
        ice::UniquePtr<ice::InputActionBuilder::Layer> _builder;
    };

} // namespace ice
