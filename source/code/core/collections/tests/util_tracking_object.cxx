#include "util_tracking_object.hxx"

bool operator==(Test_ObjectEvents const& lhs, Test_ObjectEvents const& rhs) noexcept
{
    return lhs.test_ctor == rhs.test_ctor
        && lhs.test_ctor_def == rhs.test_ctor_def
        && lhs.test_ctor_move == rhs.test_ctor_move
        && lhs.test_ctor_copy == rhs.test_ctor_copy
        && lhs.test_op_move == rhs.test_op_move
        && lhs.test_op_copy == rhs.test_op_copy;
}

Test_TrackingObject::Test_TrackingObject() noexcept
    : value{ 0 }
    , data{ .test_ctor{ 1 } }
{
}

Test_TrackingObject::Test_TrackingObject(ice::u32 value) noexcept
    : value{ value }
    , data{ .test_ctor{ 1 } }
{
}

Test_TrackingObject::Test_TrackingObject(Test_TrackingObject&& other) noexcept
    : value{ std::exchange(other.value, 0) }
    , data{
        .test_ctor{ 0 },
        .test_ctor_def{ 0 },
        .test_ctor_move{ 1 },
        .test_ctor_copy{ 0 },
        .test_op_move{ 0 },
        .test_op_copy{ 0 },
        .test_dtor{ other.data.test_dtor },
    }
{
}

Test_TrackingObject::Test_TrackingObject(Test_TrackingObject const& other) noexcept
    : value{ other.value }
    , data{
        .test_ctor{ 0 },
        .test_ctor_def{ 0 },
        .test_ctor_move{ 0 },
        .test_ctor_copy{ 1 },
        .test_op_move{ 0 },
        .test_op_copy{ 0 },
        .test_dtor{ other.data.test_dtor },
    }
{
}

Test_TrackingObject::~Test_TrackingObject() noexcept
{
    if (data.test_dtor) *data.test_dtor += 1;
}

auto Test_TrackingObject::operator=(Test_TrackingObject&& other) noexcept -> Test_TrackingObject&
{
    if (this != &other)
    {
        value = std::exchange(other.value, 0);
        data.test_op_move += 1;
    }
    return *this;
}

auto Test_TrackingObject::operator=(Test_TrackingObject const& other) noexcept -> Test_TrackingObject&
{
    if (this != &other)
    {
        value = other.value;
        data.test_op_copy += 1;
    }
    return *this;
}

void Test_TrackingObject::gather_ctors(Test_ObjectEvents& obj_events) const noexcept
{
    obj_events.test_ctor += data.test_ctor;
    obj_events.test_ctor_def += data.test_ctor_def;
    obj_events.test_ctor_move += data.test_ctor_move;
    obj_events.test_ctor_copy += data.test_ctor_copy;
}

void Test_TrackingObject::gather_operators(Test_ObjectEvents& obj_events) const noexcept
{
    obj_events.test_op_move += data.test_op_move;
    obj_events.test_op_copy += data.test_op_copy;
}

bool Test_TrackingObject::operator==(Test_TrackingObject const& obj) const noexcept
{
    return value == obj.value && data == obj.data;
}

bool Test_TrackingObject::operator==(Test_ObjectEvents const& events) const noexcept
{
    return data == events;
}
