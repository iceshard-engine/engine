/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>

#if ISP_UNIX

#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <dlfcn.h>
#pragma clang diagnostic ignored "-Wunknown-attributes"

#endif
