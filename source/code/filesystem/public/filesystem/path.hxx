#pragma once
#include <filesystem/uri.hxx>
#include <core/string.hxx>

//! \brief Functions used to operate on files.
namespace filesystem::file
{


//! \brief The URI Schema for a native file resource.
constexpr auto schema = core::cexpr::stringid_cexpr("file");


//! \brief Checks the path for existence.
bool exists(const core::String<>& path) noexcept;

//! \brief Checks the URI for existence.
bool exists(const URI& uri) noexcept;


} // namespace filesystem
