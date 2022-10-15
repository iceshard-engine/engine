#pragma once
#include <ice/math/common.hxx>
#include <ice/math/matrix.hxx>
#include <ice/math/matrix/matrix_operations.hxx>
#include <ice/math/matrix/matrix_operators.hxx>
#include <ice/math/vector.hxx>
#include <ice/math/vector/vector_operations.hxx>
#include <ice/math/vector/vector_operators.hxx>
#include <ice/math/translate.hxx>
#include <ice/math/scale.hxx>
#include <ice/math/rotate.hxx>
#include <ice/shard.hxx>

namespace ice
{

    using namespace math;

    using std::abs;

    template<>
    constexpr ShardPayloadID Constant_ShardPayloadID<ice::vec2u> = ice::shard_payloadid("ice::vec2u");

} // namespace ice
