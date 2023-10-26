#include "android_input_motion.hxx"
#include <ice/profiler.hxx>
#include <ice/log.hxx>

namespace ice::platform::android
{

    using MotionEventAction = decltype(AMOTION_EVENT_ACTION_DOWN);

    auto handle_android_motion_event(
        AInputEvent const* raw_event,
        ice::input::DeviceEventQueue& out_events
    ) noexcept -> ice::Result
    {
        using namespace ice::input;

        ice::u32 const action = AMotionEvent_getAction(raw_event);
//        [[maybe_unused]] ice::u32 const pointers = AMotionEvent_getPointerCount(raw_event);

        MotionEventAction const type = (MotionEventAction) (action & AMOTION_EVENT_ACTION_MASK);
        switch(type)
        {
            case AMOTION_EVENT_ACTION_DOWN:
                out_events.push(
                    make_device_handle(DeviceType::TouchPointer, static_cast<DeviceIndex>(AMotionEvent_getPointerId(raw_event, 0))),
                    DeviceMessage::TouchStart
                );
                ICE_LOG(LogSeverity::Debug, LogTag::Core, "Motion: Action::Down");
                break;
            case AMOTION_EVENT_ACTION_MOVE:
            {
                ice::u64 const pointer_count = AMotionEvent_getPointerCount(raw_event);
                for (ice::u64 idx = 0; idx < pointer_count; ++idx)
                {
                    ice::i32 const id = AMotionEvent_getPointerId(raw_event, idx);
                    ICE_ASSERT_CORE(id < 15);
                    out_events.push(
                        make_device_handle(DeviceType::TouchPointer, static_cast<DeviceIndex>(id)),
                        DeviceMessage::TouchPositionX,
                        AMotionEvent_getX(raw_event, idx)
                    );
                    out_events.push(
                        make_device_handle(DeviceType::TouchPointer, static_cast<DeviceIndex>(id)),
                        DeviceMessage::TouchPositionY,
                        AMotionEvent_getY(raw_event, idx)
                    );
                }
                IPT_MESSAGE("Motion: Action::Move");
                break;
            }
            case AMOTION_EVENT_ACTION_UP:
                out_events.push(
                    make_device_handle(DeviceType::TouchPointer, static_cast<DeviceIndex>(AMotionEvent_getPointerId(raw_event, 0))),
                    DeviceMessage::TouchEnd
                );
                ICE_LOG(LogSeverity::Debug, LogTag::Core, "Motion: Action::Up");
                break;

            // TODO actions
            case AMOTION_EVENT_ACTION_OUTSIDE: break;
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
            {
                ice::u16 const pointer = ((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
                ice::i32 const id = AMotionEvent_getPointerId(raw_event, pointer);
                ICE_ASSERT_CORE(id < 15);
                out_events.push(
                    make_device_handle(DeviceType::TouchPointer, static_cast<DeviceIndex>(id)),
                    DeviceMessage::TouchStart
                );
                ICE_LOG(LogSeverity::Debug, LogTag::Core, "Motion: Pointer::Down");
                break;
            }
            case AMOTION_EVENT_ACTION_POINTER_UP:
            {
                ice::u16 const pointer = ((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
                ice::i32 const id = AMotionEvent_getPointerId(raw_event, pointer);
                ICE_ASSERT_CORE(id < 15);
                out_events.push(
                    make_device_handle(DeviceType::TouchPointer, static_cast<DeviceIndex>(id)),
                    DeviceMessage::TouchEnd
                );
                ICE_LOG(LogSeverity::Debug, LogTag::Core, "Motion: Pointer::Up");
                break;
            }

            // Unhandled actions
            case AMOTION_EVENT_ACTION_SCROLL: break;
            case AMOTION_EVENT_ACTION_BUTTON_PRESS:
            case AMOTION_EVENT_ACTION_BUTTON_RELEASE: return I_ButtonActionIgnored;
            case AMOTION_EVENT_ACTION_HOVER_ENTER:
            case AMOTION_EVENT_ACTION_HOVER_EXIT:
            case AMOTION_EVENT_ACTION_HOVER_MOVE: return I_HooverActionIgnored;
        }

        return ice::Res::Success;
    }

} // namespace ice::platform::android
