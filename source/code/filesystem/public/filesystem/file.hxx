#pragma once
#include <resource/uri.hxx>
#include <core/cexpr/stringid.hxx>

namespace filesystem::file
{


//! \brief A file identifier.
enum class file_identifier : uint64_t;

//! \brief A special case for a invalid identifier.
extern file_identifier file_invalid_identifier;


//! \brief Search for the given file.
//! \returns A file identifier.
//auto find(const resouce::URI& uri) noexcept -> file_identifier;


} // namespace filesystem
