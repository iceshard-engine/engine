#pragma once

#ifdef MEMSYS_EXPORT
#   ifdef _DLL
#       define MEMSYS_API __declspec(dllexport)
#   else
#       define MEMSYS_API __declspec(dllimport)
#   endif
#else
#   define MEMSYS_API
#endif
