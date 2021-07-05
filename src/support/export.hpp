#pragma once
#ifndef SHARED_EXPORT_H_
#define SHARED_EXPORT_H_

// Definition for Win32 Export directives.

#include "config-lite.hpp"

#if defined (AS_STATIC_LIBS)
// todo : isn't support
# if !defined (HAS_DLL)
#   define HAS_DLL 0
# endif // HAS_DLL
#else
# if !defined (HAS_DLL)
#   define HAS_DLL 1
# endif // HAS_DLL
#endif // AS_STATIC_LIBS

#if defined (HAS_DLL)
#  if (HAS_DLL == 1)
#    if defined (BUILD_DLL)
#      define DECL_Export DECL_Proper_Export_Flag
#    else
#      define DECL_Export DECL_Proper_Import_Flag
#    endif // BUILD_DLL
#  else
#    define DECL_Export
#  endif   // HAS_DLL == 1
#else
#  define DECL_Export
#endif // HAS_DLL

#endif // SHARED_EXPORT_H_