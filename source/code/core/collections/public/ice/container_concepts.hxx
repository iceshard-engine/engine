/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>

namespace ice
{

    template<typename Node>
    concept LinkedListNode = requires(Node node) {
        { node.next } -> std::convertible_to<Node*>;
    };

} // namespace ice
