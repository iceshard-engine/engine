/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <ice/stringid.hxx>

namespace ice
{

    //! \brief Small value object able to pass up to 8 bytes of data accross the engine.
    //!
    //! \detail A single shard is always 16 bytes in size.
    //!   The first 4 bytes represent the name <em>(ice::ShardID)</em> and the next four the typeid <em>(ice::ShardPayloadID)</em> of the carried data.
    //!   Because this object is designed to only pass small values it's mostly used to carry pointers to constant objects living in a frame.
    //!
    //! \remark This object can also seen as small 'event' type that carries data. It's also used as such in various places.
    struct Shard;

    //! \brief Represents the name of a ice::Shard. Created from a utf8 string value.
    struct ShardID;

    //! \brief Represents the typeid of a ice::Shard. Created from a utf8 string value.
    struct ShardPayloadID;

    //! \brief Returns the ice::ShardID value of a shard.
    //! \note It's better to use this function, instead of accessing the Shard::id member directly to avoid breaking changes.
    constexpr auto shardid(ice::Shard shard) noexcept -> ice::ShardID;

    //! \brief Creates a ice::ShardID value from a utf8 string.
    //!
    //! \detail The passed value can contain the name and typeid.
    //!   To do so the names need to be separated by a '`' <em>(backquote)</em> character. For example.: `my-shard`ice::u32`
    //!
    //! \note Even if you create a ice::ShardID with a typeid that is not enabled, it will not allow you to create a shard with such a value.
    constexpr auto shardid(std::string_view definition) noexcept -> ice::ShardID;

    //! \brief Creates a ice::Shard value from ice::ShardID. Clears the payload ID from the created shard.
    constexpr auto shard(ice::ShardID id) noexcept -> ice::Shard;

    //! \brief Creates a ice::Shard value from a utf8 string and the given value.
    //!
    //! \detail The function returns the final shard if both the definition and the typeid of the given value match.
    //!   Otherwise the shard fails to create.
    //!
    //! \param[in] definition Follows the same rules described in ice::shardid(std::u8_string_view).
    //! \param[in] value A value with a type enabled for sharding with ice::Constant_ShardPayloadID.
    template<typename T>
    constexpr auto shard(std::string_view definition, T value);

    //! \brief Creates a ice::Shard value from ice::ShardID and the given value.
    //!
    //! \detail The function returns the final shard if both the shardid and the typeid of the given value match.
    //!   Otherwise the shard fails to create.
    //!
    //! \param[in] id ShardID used to create the shard.
    //! \param[in] value A value with a type enabled for sharding with ice::Constant_ShardPayloadID.
    template<typename T>
    constexpr auto shard(ice::ShardID id, T payload) noexcept -> ice::Shard;

    //! \brief Tries to read the value from the given shard.
    //!
    //! \param[out] payload A reference where the payload should be stored.
    //! \returns false If the types are not matching, or there is no payload value in the shard.
    template<typename T>
    constexpr bool shard_inspect(ice::Shard shard, T& payload) noexcept;

    //! \brief Reads the value from the given shard or returns the fallback value.
    //!
    //! \returns The payload value or the fallback value.
    template<typename T>
    constexpr auto shard_shatter(ice::Shard shard, T fallback) noexcept -> T;

    constexpr auto operator""_shard(ice::utf8 const* str, size_t size) noexcept -> ice::Shard;
    constexpr auto operator""_shardid(ice::utf8 const* str, size_t size) noexcept -> ice::ShardID;


    // IMPLEMENTATION DETAILS

    namespace detail
    {

        struct ShardName
        {
            using TypeTag = ice::StrongValue;

            ice::u32 value;
        };

        struct ShardPayload
        {
            using TypeTag = ice::StrongValue;

            ice::u64 value;
        };

    } // namespace detail

    struct ShardPayloadID
    {
        using TypeTag = ice::StrongValue;

        ice::u32 value;
    };

    struct ShardID
    {
        ice::detail::ShardName name;
        ice::ShardPayloadID payload;
    };

    struct Shard
    {
        ice::ShardID id;
        ice::detail::ShardPayload payload;

        constexpr operator ice::ShardID() const noexcept
        {
            return id;
        }
    };

    static constexpr ice::Shard Shard_Invalid{ .id = { }, .payload = { } };

    constexpr auto shard_payloadid(std::string_view sv)  noexcept -> ice::ShardPayloadID
    {
        namespace mm3 = ice::detail::murmur3_hash;

        mm3::mm3_x86_h32 const hash = mm3::cexpr_murmur3_x86_32(
            sv, ice::config::ShardPayloadID_DefaultSeed
        );
        return ice::ShardPayloadID{ hash.h[0] };
    }

    constexpr auto shardid(std::string_view sv) noexcept -> ice::ShardID
    {
        namespace mm3 = ice::detail::murmur3_hash;

        size_t const payload_id_pos = sv.find_first_of('`');
        if (payload_id_pos == std::string_view::npos)
        {
            mm3::mm3_x86_h32 const hash = mm3::cexpr_murmur3_x86_32(sv, ice::config::ShardName_DefaultSeed);

            return ice::ShardID{
                .name = { hash.h[0] },
                .payload = { }
            };
        }
        else
        {
            mm3::mm3_x86_h32 const name_hash = mm3::cexpr_murmur3_x86_32(
                sv.substr(0, payload_id_pos),
                ice::config::ShardName_DefaultSeed
            );

            return ice::ShardID{
                .name = { name_hash.h[0] },
                .payload = shard_payloadid(sv.substr(payload_id_pos + 1))
            };
        }
    }

    constexpr auto shardid(ice::Shard shard) noexcept -> ice::ShardID
    {
        return shard.id;
    }

    constexpr auto operator""_shardid(char const* str, size_t size) noexcept -> ice::ShardID
    {
        return ice::shardid({ str, size });
    }

    constexpr static ice::ShardPayloadID ShardPayloadID_NotSet = { 0 };

    template<typename T>
    concept AllowedAsShardPayloadID = std::is_trivially_copyable_v<T> && sizeof(T) <= 8;

    template<typename T> requires AllowedAsShardPayloadID<T>
    constexpr static ice::ShardPayloadID Constant_ShardPayloadID = ice::ShardPayloadID_NotSet;

    template<typename T>
    concept HasShardPayloadID = Constant_ShardPayloadID<T> != ice::ShardPayloadID_NotSet;


    namespace detail
    {

        template<typename T> requires HasShardPayloadID<T>
        constexpr auto shard_payload(T payload) noexcept -> ice::detail::ShardPayload
        {
            if constexpr (sizeof(T) == sizeof(ice::Shard::payload))
            {
                return std::bit_cast<ShardPayload>(payload);
            }
            else
            {
                struct PayloadBitCastHelper
                {
                    T value;
                    char bytes[sizeof(ice::Shard::payload) - sizeof(T)];
                } temp{ .value = payload, .bytes = { } };

                return std::bit_cast<ShardPayload>(temp);
            }
        }

        template<typename T> requires HasShardPayloadID<T>
        constexpr auto shard_value(ShardPayload payload) noexcept -> T
        {
            if constexpr (sizeof(T) == sizeof(ice::Shard::payload))
            {
                return std::bit_cast<T>(payload);
            }
            else
            {
                struct PayloadBitCastHelper
                {
                    T value;
                    char bytes[sizeof(ice::Shard::payload) - sizeof(T)];
                } temp{ std::bit_cast<PayloadBitCastHelper>(payload) };

                return temp.value;
            }
        }

    } // namespace detail

    // SHARD CREATION

    constexpr auto shard(ice::ShardID id) noexcept -> ice::Shard
    {
        return ice::Shard{ .id = id };
    }

    template<typename T> requires ice::HasShardPayloadID<T>
    constexpr auto shard(std::string_view definition, T value) noexcept -> ice::Shard
    {
        ice::Shard result{ };
        ice::ShardID const id = ice::shardid(definition);

        if (std::is_constant_evaluated() == false)
        {
            ICE_ASSERT_CORE(id.payload == ice::Constant_ShardPayloadID<T> || id.payload == ice::ShardPayloadID_NotSet);
        }

        if (id.payload == ice::Constant_ShardPayloadID<T> || id.payload == ice::ShardPayloadID_NotSet)
        {
            result.id = id;
            result.id.payload = ice::Constant_ShardPayloadID<T>;
            result.payload = ice::detail::shard_payload(value);
        }
        return result;
    }

    template<typename T> requires ice::HasShardPayloadID<T>
    constexpr auto shard(ice::ShardID id, T value) noexcept -> ice::Shard
    {
        ice::Shard result{ };

        if (std::is_constant_evaluated() == false)
        {
            ICE_ASSERT_CORE(id.payload == ice::Constant_ShardPayloadID<T> || id.payload == ice::ShardPayloadID_NotSet);
        }

        if (id.payload == ice::Constant_ShardPayloadID<T> || id.payload == ice::ShardPayloadID_NotSet)
        {
            result.id = id;
            result.id.payload = ice::Constant_ShardPayloadID<T>;
            result.payload = ice::detail::shard_payload(value);
        }
        return result;
    }

    constexpr auto operator""_shard(char const* str, size_t size) noexcept -> ice::Shard
    {
        return ice::shard(ice::shardid({ str, size }));
    }

    template<typename T> requires ice::HasShardPayloadID<T>
    constexpr auto operator|(ice::Shard shard, T payload) noexcept -> ice::Shard
    {
        return ice::shard(shard.id, payload);
    }

    template<typename T> requires ice::HasShardPayloadID<T>
    constexpr auto operator|(ice::ShardID shardid, T payload) noexcept -> ice::Shard
    {
        return ice::shard(shardid, payload);
    }

    constexpr auto operator==(ice::ShardID left, ice::ShardID right) noexcept -> bool
    {
        if (left.name == right.name)
        {
            return left.payload == ice::ShardPayloadID_NotSet
                || right.payload == ice::ShardPayloadID_NotSet
                || right.payload == left.payload;
        }
        return false;
    }

    constexpr auto operator==(ice::Shard left, ice::Shard right) noexcept -> bool
    {
        return left.id == right.id;
    }

    constexpr auto operator==(ice::Shard left, ice::ShardID right) noexcept -> bool
    {
        return left.id == right;
    }

    // SHARD INSPECTION

    template<typename T> requires ice::HasShardPayloadID<T>
    constexpr bool shard_inspect(ice::Shard shard, T& value) noexcept
    {
        if (ice::Constant_ShardPayloadID<T> == shard.id.payload)
        {
            value = ice::detail::shard_value<T>(shard.payload);
            return true;
        }
        return false;
    }

    template<typename T> requires ice::HasShardPayloadID<T>
    constexpr auto shard_shatter(ice::Shard shard, T fallback) noexcept -> T
    {
        if (ice::Constant_ShardPayloadID<T> == shard.id.payload)
        {
            return ice::detail::shard_value<T>(shard.payload);
        }
        return fallback;
    }

    // COMMON PAYLOAD TYPES

    template<>
    constexpr inline ice::ShardPayloadID Constant_ShardPayloadID<bool> = ice::shard_payloadid("bool");

    template<>
    constexpr inline ice::ShardPayloadID Constant_ShardPayloadID<ice::i32> = ice::shard_payloadid("ice::i32");

    template<>
    constexpr inline ice::ShardPayloadID Constant_ShardPayloadID<ice::u32> = ice::shard_payloadid("ice::u32");

    template<>
    constexpr inline ice::ShardPayloadID Constant_ShardPayloadID<ice::f32> = ice::shard_payloadid("ice::f32");

    template<>
    constexpr inline ice::ShardPayloadID Constant_ShardPayloadID<ice::i64> = ice::shard_payloadid("ice::i64");

    template<>
    constexpr inline ice::ShardPayloadID Constant_ShardPayloadID<ice::u64> = ice::shard_payloadid("ice::u64");

    template<>
    constexpr inline ice::ShardPayloadID Constant_ShardPayloadID<ice::f64> = ice::shard_payloadid("ice::f64");

    template<>
    constexpr inline ice::ShardPayloadID Constant_ShardPayloadID<char const*> = ice::shard_payloadid("char const*");

    template<>
    constexpr inline ice::ShardPayloadID Constant_ShardPayloadID<std::string_view const*> = ice::shard_payloadid("std::string_view const*");

    template<>
    constexpr inline ice::ShardPayloadID Constant_ShardPayloadID<ice::StringID_Hash> = ice::shard_payloadid("ice::StringID_Hash");


    constexpr auto hash(ice::ShardID shardid) noexcept -> ice::u64
    {
        return ice::bit_cast<ice::u64>(shardid);
    }

    constexpr auto hash32(ice::ShardID shardid) noexcept -> ice::u32
    {
        return shardid.name.value ^ shardid.payload.value;
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
        static constexpr ice::ShardID shardid_test_1_from_shard = ice::shardid(shard_with_payload_u32);

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
