#pragma once
#include <ice/string/string.hxx>
#include <ice/input/input_event.hxx>
#include <ice/input/input_controller.hxx>
#include <ice/input/input_keyboard.hxx>
#include <ice/input/input_mouse.hxx>
#include <ice/math.hxx>
#include <ice/span.hxx>

namespace ice
{

    enum class InputActionCheck : ice::u8;
    enum class InputActionSourceEvent : ice::u8;
    enum class InputActionSourceType : ice::u8;
    enum class InputActionBehavior : ice::u8;
    enum class InputActionCondition : ice::u8;
    enum class InputActionStep : ice::u8;
    enum class InputActionModifier : ice::u8;

    // TODO: Rename
    enum class InputActionData : ice::u8;

    struct InputActionSource;
    struct InputActionSourceInfo;
    struct InputAction;
    struct InputActionInfo;
    struct InputActionRuntime;

    struct InputActionModifierData;
    struct InputActionConditionData;
    struct InputActionStepData;

    class InputActionStack;
    class InputActionLayer;
    class InputActionLayerBuilder;
    class InputActionExecutor;

} // namespace ice

template<>
constexpr ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::InputAction const*> = ice::shard_payloadid("ice::InputAction const*");
