#pragma once

#ifdef FILESYSTEM_EXPORT
#    ifdef _DLL
#        define FILESYSTEM_API __declspec(dllexport)
#    else
#        define FILESYSTEM_API
#    endif
#else
#    define FILESYSTEM_API __declspec(dllimport)
#endif
