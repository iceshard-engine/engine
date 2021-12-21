#pragma once
#include <ice/base.hxx>
#include <ice/hash.hxx>
#include <string_view>

namespace ice
{

    enum class ShardName : ice::u32;
    enum class PayloadID : ice::u32;
    enum class Payload : ice::u64;
    enum class ShardID : ice::u64;

    struct Shard
    {
        ice::ShardName name;
        ice::PayloadID payload_id;
        ice::Payload payload;
    };


    constexpr auto shard_name(std::string_view name) noexcept -> ice::ShardName;

    constexpr auto payload_id(std::string_view name) noexcept -> ice::PayloadID;

    constexpr auto shard_id(std::string_view sv) noexcept -> ice::ShardID;

    constexpr auto shard_id(ice::ShardName name, ice::PayloadID payload_id) noexcept -> ice::ShardID;

    constexpr auto shard_id(ice::Shard shard) noexcept -> ice::ShardID;


    constexpr auto shard_create(ice::ShardID shard_id) noexcept -> ice::Shard;

    constexpr auto shard_create(std::string_view sv) noexcept -> ice::Shard;

    template<typename T>
    constexpr auto shard_create(std::string_view sv, T value) noexcept -> ice::Shard;

    template<typename T>
    constexpr auto shard_create(ice::Shard sv, T value) noexcept -> ice::Shard;

    template<typename T>
    constexpr bool shard_inspect(ice::Shard shard, T& value) noexcept;

    template<typename T>
    inline auto shard_shatter(ice::Shard shard) noexcept -> T;

    constexpr auto shard_transform(ice::Shard old_shard, ice::Shard new_shard) noexcept -> ice::Shard;


    enum class ShardName : ice::u32
    {
        Invalid = 0x0
    };

    enum class PayloadID : ice::u32
    {
        NotSet = 0x0
    };

    enum class Payload : ice::u64
    {
        Empty = 0x0
    };

    enum class ShardID : ice::u64
    {
        Invalid = 0x0
    };

    static constexpr ice::ShardID ShardID_Invalid = ShardID::Invalid;

    static constexpr ice::Shard Shard_Invalid{ ShardName::Invalid, PayloadID::NotSet, Payload::Empty };


    constexpr auto shard_name(std::string_view name) noexcept -> ice::ShardName
    {
        return ShardName{ ice::hash32(name) };
    }

    constexpr auto payload_id(std::string_view name) noexcept -> ice::PayloadID
    {
        return PayloadID{ ice::hash32(name) };
    }

    constexpr auto shard_id(std::string_view sv) noexcept -> ice::ShardID
    {
        size_t const payload_id_pos = sv.find_first_of('`');
        if (payload_id_pos == std::string_view::npos)
        {
            return ice::shard_id(ice::shard_name(sv), PayloadID::NotSet);
        }
        else
        {
            return ice::shard_id(
                ice::shard_name(sv.substr(0, payload_id_pos)),
                ice::payload_id(sv.substr(payload_id_pos + 1))
            );
        }
    }

    constexpr auto shard_id(ice::ShardName name, ice::PayloadID payload_id) noexcept -> ice::ShardID
    {
        ice::u64 id_value = static_cast<ice::u32>(name);
        id_value <<= 32;
        id_value |= static_cast<ice::u32>(payload_id);
        return static_cast<ice::ShardID>(id_value);
    }

    constexpr auto shard_id(ice::Shard shard) noexcept -> ice::ShardID
    {
        return shard_id(shard.name, shard.payload_id);
    }

    namespace detail::stringid_type_v2
    {

        enum class StringID_Hash : uint64_t;

    } // namespace detail::stringid_type_v2

    namespace detail
    {

        template<typename T>
        constexpr ice::PayloadID Constant_ShardPayloadID = PayloadID::NotSet;

        template<>
        constexpr ice::PayloadID Constant_ShardPayloadID<ice::i32> = ice::payload_id("ice::i32");

        template<>
        constexpr ice::PayloadID Constant_ShardPayloadID<ice::u32> = ice::payload_id("ice::u32");

        template<>
        constexpr ice::PayloadID Constant_ShardPayloadID<ice::f32> = ice::payload_id("ice::f32");

        template<>
        constexpr ice::PayloadID Constant_ShardPayloadID<ice::i64> = ice::payload_id("ice::i64");

        template<>
        constexpr ice::PayloadID Constant_ShardPayloadID<ice::u64> = ice::payload_id("ice::u64");

        template<>
        constexpr ice::PayloadID Constant_ShardPayloadID<ice::f64> = ice::payload_id("ice::f64");

        template<>
        constexpr ice::PayloadID Constant_ShardPayloadID<ice::math::vec2i> = ice::payload_id("ice::math::vec2i");

        template<>
        constexpr ice::PayloadID Constant_ShardPayloadID<ice::math::vec2u> = ice::payload_id("ice::math::vec2u");

        template<>
        constexpr ice::PayloadID Constant_ShardPayloadID<ice::math::vec2f> = ice::payload_id("ice::math::vec2f");

        template<>
        constexpr ice::PayloadID Constant_ShardPayloadID<ice::detail::stringid_type_v2::StringID_Hash> = ice::payload_id("ice::StringID_Hash");

    } // namespace detail

    constexpr auto shard_create(ice::ShardID shard_id) noexcept -> ice::Shard
    {
        ice::u64 const id_value = static_cast<ice::u64>(shard_id);
        ice::PayloadID const payload_id = ice::PayloadID{ id_value & 0x00000000'ffffffff };
        ice::ShardName const shard_name = ice::ShardName{ (id_value & 0xffffffff'00000000) >> 32 };
        return ice::Shard{
            .name = shard_name,
            .payload_id = payload_id,
            .payload = Payload::Empty,
        };
    }

    constexpr auto shard_create(std::string_view sv) noexcept -> ice::Shard
    {
        return ice::Shard{
            .name = ice::shard_name(sv),
            .payload_id = PayloadID::NotSet,
            .payload = Payload::Empty,
        };
    }

    template<typename T>
    constexpr auto shard_create(std::string_view sv, T payload) noexcept -> ice::Shard
    {
        static_assert(sizeof(T) <= sizeof(ice::Shard::payload), "The given payload is bigger than a shard can have attached.");
        static_assert(ice::detail::Constant_ShardPayloadID<T> != PayloadID::NotSet, "The given type cannot be used to attach a shard payload.");
        if constexpr (std::is_pointer_v<T> == false)
        {
            return {
                .name = ice::shard_name(sv),
                .payload_id = ice::detail::Constant_ShardPayloadID<T>,
                .payload = static_cast<ice::Payload>(payload)
            };
        }
        else
        {
            return {
                .name = ice::shard_name(sv),
                .payload_id = ice::detail::Constant_ShardPayloadID<T>,
                .payload = *reinterpret_cast<ice::Payload const*>(ice::addressof(payload))
            };
        }
    }

    template<typename T>
    constexpr auto shard_create(ice::Shard shard, T payload) noexcept -> ice::Shard
    {
        static_assert(sizeof(T) <= sizeof(ice::Shard::payload), "The given payload is bigger than a shard can have attached.");
        static_assert(ice::detail::Constant_ShardPayloadID<T> != PayloadID::NotSet, "The given type cannot be used to attach a shard payload.");
        if constexpr (std::is_pointer_v<T> == false && std::is_class_v<T> == false)
        {
            return {
                .name = shard.name,
                .payload_id = ice::detail::Constant_ShardPayloadID<T>,
                .payload = static_cast<ice::Payload>(payload)
            };
        }
        else
        {
            return {
                .name = shard.name,
                .payload_id = ice::detail::Constant_ShardPayloadID<T>,
                .payload = *reinterpret_cast<ice::Payload const*>(ice::addressof(payload))
            };
        }
    }

    template<typename T> requires (std::is_pointer_v<T> == false && std::is_class_v<T> == false)
    constexpr bool shard_inspect(ice::Shard shard, T& value) noexcept
    {
        static_assert(ice::detail::Constant_ShardPayloadID<T> != PayloadID::NotSet, "The given type cannot be used to inspect a shard object.");
        if (ice::detail::Constant_ShardPayloadID<T> == shard.payload_id)
        {
            value = static_cast<T>(shard.payload);
            return true;
        }
        return false;
    }

    template<typename T> requires (std::is_pointer_v<T> == false && std::is_class_v<T>)
    constexpr bool shard_inspect(ice::Shard shard, T& value) noexcept
    {
        static_assert(ice::detail::Constant_ShardPayloadID<T> != PayloadID::NotSet, "The given type cannot be used to inspect a shard object.");
        if (ice::detail::Constant_ShardPayloadID<T> == shard.payload_id)
        {
            value = *reinterpret_cast<T const*>(ice::addressof(shard.payload));
            return true;
        }
        return false;
    }

    template<typename T>
    inline bool shard_inspect(ice::Shard shard, T*& value) noexcept
    {
        static_assert(ice::detail::Constant_ShardPayloadID<T*> != PayloadID::NotSet, "The given type cannot be used to inspect a shard object.");
        if (ice::detail::Constant_ShardPayloadID<T*> == shard.payload_id)
        {
            value = reinterpret_cast<T*>(static_cast<uptr>(shard.payload));
            return true;
        }
        return false;
    }

    template<typename T>
    inline auto shard_shatter(ice::Shard shard) noexcept -> T
    {
        static_assert(ice::detail::Constant_ShardPayloadID<T> != PayloadID::NotSet, "The given type cannot be used to shatter a shard object.");
        return *reinterpret_cast<T const*>(ice::addressof(shard.payload));
    }

    constexpr auto shard_transform(ice::Shard source_shard, ice::Shard destination_shard) noexcept -> ice::Shard
    {
        return {
            .name = destination_shard.name,
            .payload_id = source_shard.payload_id,
            .payload = source_shard.payload
        };
    }


    constexpr auto operator""_shardid(const char* str, size_t size) noexcept
    {
        return ice::shard_id({ str, size });
    }

    constexpr auto operator""_shard(const char* str, size_t size) noexcept
    {
        return ice::shard_create(ice::shard_id({ str, size }));
    }

    constexpr auto operator""_shard_name(const char* str, size_t size) noexcept
    {
        return ice::shard_create(ice::shard_id({ str, size })).name;
    }


    template<typename T>
    constexpr auto operator|(ice::Shard shard, T payload) noexcept -> ice::Shard
    {
        return ice::shard_create(shard, payload);
    }

    template<typename T>
    constexpr auto operator|(ice::Shard shard, T* payload) noexcept -> ice::Shard
    {
        return ice::shard_create(shard, payload);
    }

    constexpr auto operator>>(ice::Shard left, ice::ShardID right) noexcept -> ice::Shard
    {
        return ice::shard_transform(left, ice::shard_create(right));
    }

    constexpr auto operator>>(ice::Shard left, ice::Shard right) noexcept -> ice::Shard
    {
        return ice::shard_transform(left, right);
    }

    constexpr auto operator==(ice::Shard left, ice::Shard right) noexcept -> bool
    {
        if (left.name == right.name)
        {
            return left.payload_id == PayloadID::NotSet || right.payload_id == PayloadID::NotSet || right.payload_id == left.payload_id;
        }
        return false;
    }

    constexpr auto operator==(ice::Shard left, ice::ShardID right) noexcept -> bool
    {
        return left == ice::shard_create(right);
    }

    constexpr auto operator!=(ice::Shard left, ice::Shard right) noexcept -> bool
    {
        return !(left == right);
    }


    namespace _validate
    {

        static constexpr ice::Shard shard_without_payload = "test/shard"_shard;
        static constexpr ice::Shard shard_with_payload_u32 = "test/shard"_shard | ice::u32{ 42 };
        static constexpr ice::Shard shard_with_payload_i32 = "test/shard"_shard | ice::i32{ 42 };

        static constexpr ice::Shard shard2_without_payload = "test/shard2"_shard;
        static constexpr ice::Shard shard2_with_payload_u32 = "test/shard2"_shard | ice::u32{ 42 };
        static constexpr ice::Shard shard2_with_payload_i32 = "test/shard2"_shard | ice::i32{ 42 };

        static constexpr ice::ShardID shardid_test_1 = "test/shard`ice::u32"_shardid;
        static constexpr ice::ShardID shardid_test_1_from_shard = ice::shard_id(shard_with_payload_u32);

        static_assert(shardid_test_1 == shardid_test_1_from_shard, "ShardID creation is not valid!");

        static_assert(shard_without_payload == shard_without_payload, "Assert: Shards are equal to themselfs.");
        static_assert(shard_with_payload_u32 == shard_with_payload_u32, "Assert: Shards are equal to themselfs.");

        static_assert(shard_without_payload == shard_with_payload_u32, "Assert: Shards without payload are equal to similar Shards with payload.");
        static_assert(shard_without_payload == shard_with_payload_i32, "Assert: Shards without payload are equal to similar Shards with payload.");
        static_assert(shard_with_payload_u32 == shard_without_payload, "Assert: Shards without payload are equal to similar Shards with payload.");
        static_assert(shard_with_payload_i32 == shard_without_payload, "Assert: Shards without payload are equal to similar Shards with payload.");

        static_assert(shard_with_payload_i32 != shard_with_payload_u32, "Assert: Shards with different payloads are not equal.");
        static_assert(shard_with_payload_u32 != shard_with_payload_i32, "Assert: Shards with different payloads are not equal.");

        static_assert(shard_without_payload != shard2_without_payload, "Assert: Different Shards are not equal.");

        static_assert(shard2_with_payload_u32 != shard_with_payload_u32, "Assert: Different Shards with same payloads are not equal.");
        static_assert(shard2_with_payload_i32 != shard_with_payload_i32, "Assert: Different Shards with same payloads are not equal.");

        static_assert(shard2_with_payload_u32 != shard_with_payload_i32, "Assert: Different Shards with different payloads are not equal.");
        static_assert(shard2_with_payload_i32 != shard_with_payload_u32, "Assert: Different Shards with different payloads are not equal.");

    } // namespace detail

} // namespace ice
