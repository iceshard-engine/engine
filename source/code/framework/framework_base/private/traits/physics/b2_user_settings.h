#pragma once
#include <ice/base.hxx>
#include <ice/allocator.hxx>
#include <ice/log.hxx>
#include <ice/entity/entity.hxx>

#include <box2d/b2_types.h>
#include <box2d/b2_api.h>

#include <stdarg.h>
// Tunable Constants

/// You can use this to change the length scale used by your game.
/// For example for inches you could use 39.4.
#define b2_lengthUnitsPerMeter 1.0f

/// The maximum number of vertices on a convex polygon. You cannot increase
/// this too much because b2BlockAllocator has a maximum object size.
#define b2_maxPolygonVertices    8

// User data

/// You can define this to inject whatever data you want in b2Body
struct B2_API b2BodyUserData
{
    ice::Entity entity;
};

/// You can define this to inject whatever data you want in b2Fixture
struct B2_API b2FixtureUserData
{
    ice::uptr userdata = 0;
};

/// You can define this to inject whatever data you want in b2Joint
struct B2_API b2JointUserData
{
    ice::uptr userdata = 0;
};

inline void* b2Alloc(void* alloc, ice::i32 size)
{
    return reinterpret_cast<ice::Allocator*>(alloc)->allocate(static_cast<ice::u32>(size));
}

inline void b2Free(void* alloc, void* memory)
{
    reinterpret_cast<ice::Allocator*>(alloc)->deallocate(memory);
}

/// Implement this to use your own logging.
inline void b2Log(const char* string, ...)
{
    char final_message[512];

    va_list args;
    va_start(args, string);
    vsnprintf_s(final_message, _TRUNCATE, string, args);
    va_end(args);

    ICE_LOG(
        ice::LogSeverity::Debug, ice::LogTag::Engine,
        "<Box2D> {}", final_message
    );
}
