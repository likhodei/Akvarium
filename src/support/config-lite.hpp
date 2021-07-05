#pragma once

#include "pre.hpp"

// ============================================================================
// Macros for controlling the lifetimes of dlls loaded by DLL--including
// all dlls loaded via the Service Config framework.

// Per-process policy that unloads dlls eagerly.
#define DLL_UNLOAD_POLICY_PER_PROCESS 0
// Apply policy on a per-dll basis.  If the dll doesn't use one of the macros
// below, the current per-process policy will be used.
#define DLL_UNLOAD_POLICY_PER_DLL 1
// Don't unload dll when refcount reaches zero, i.e., wait for either an
// explicit unload request or program exit.
#define DLL_UNLOAD_POLICY_LAZY 2
// Default policy allows dlls to control their own destinies, but will
// unload those that don't make a choice eagerly.
#define DLL_UNLOAD_POLICY_DEFAULT DLL_UNLOAD_POLICY_PER_DLL

#if defined(_WIN32) && !defined(__GNUC__) // windows
#define DECL_Proper_Export_Flag __declspec (dllexport)
#define DECL_Proper_Import_Flag __declspec (dllimport)
#else // Unix, Linux
#define __stdcall
#define DECL_Proper_Import_Flag
#define DECL_Proper_Export_Flag
#endif

#include "post.hpp"