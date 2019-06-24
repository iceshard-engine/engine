#pragma once

#define ACCESS_KEY AccessKey
#define DEFINE_ACCESS_KEY(friend_class) \
    class ACCESS_KEY { ACCESS_KEY() { } ACCESS_KEY(const ACCESS_KEY&) = default; friend class friend_class; }
