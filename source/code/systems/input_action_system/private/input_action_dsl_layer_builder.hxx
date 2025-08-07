#pragma once
#include "input_action_script.hxx"
#include <ice/input_action_layer_builder.hxx>

namespace ice
{

    class InputActionScriptParser : public ice::asl::ActionInputParserEvents
    {
    public:
        InputActionScriptParser(ice::Allocator& alloc) noexcept;

        virtual void on_layer_parsed(ice::UniquePtr<ice::InputActionLayer> layer) noexcept = 0;

        void visit(arctic::SyntaxNode<ice::asl::Layer> node) noexcept override;

    private:
        void visit_source(
            ice::InputActionBuilder::Layer& layer,
            arctic::SyntaxNode<ice::asl::LayerSource> node
        ) noexcept;

        void visit_action(
            ice::InputActionBuilder::Layer& layer,
            arctic::SyntaxNode<ice::asl::LayerAction> node
        ) noexcept;

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

    private:
        ice::Allocator& _allocator;
    };

} // namespace ice
