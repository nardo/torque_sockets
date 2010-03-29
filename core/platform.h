// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

#if defined(__native_client__)
#define PLATFORM_NACL
#define FN_CDECL
#include "system_includes_nacl.h"

#elif defined(__WIN32__) || defined(_WIN32) || defined(__CYGWIN__)
static const char *operating_system_string = "Win32";
#define PLATFORM_WIN32
#define FN_CDECL __cdecl
#include "system_includes_win32.h"

#elif defined(linux)
static const char *operating_system_string = "Linux";
#define PLATFORM_LINUX
#define FN_CDECL
#include "system_includes_linux.h"

#elif defined(__APPLE__)
static const char *operating_system_string = "Mac OSX";
#define PLATFORM_MAC_OSX
#define FN_CDECL
#include "system_includes_mac_osx.h"

#else
#error "Unsupported operating system."
#endif

#if defined(_MSC_VER)
#define COMPILER_VISUALC
static const char *compiler_string = "VisualC++";

#elif defined(__MWERKS__)
#define COMPILER_MWERKS
static const char *compiler_string = "Metrowerks";

#elif defined(__GNUC__)
#define COMPILER_GCC
static const char *compiler_string = "GNU C Compiler";

#else
#  error "Unknown Compiler"
#endif

#if defined(_M_IX86) || defined(i386) || defined(__x86_64__)
	static const char *cpu_string = "Intel x86";
	#define CPU_X86
    #ifndef LITTLE_ENDIAN
	#define LITTLE_ENDIAN
    #endif

	#ifdef x86_64
	#define CPU_64BIT
	#else
	#define CPU_32BIT
	#endif

	#if defined (__GNUC__)
		#define INLINE_ASM_STYLE_GCC_X86
	#elif defined (__MWERKS__)
		#define INLINE_ASM_STYLE_MWERKS_X86
	#else
		#define INLINE_ASM_STYLE_VC_X86
	#endif
#elif defined(__ppc__) || defined(__powerpc__) || defined(PPC)
	static const char *cpu_string = "PowerPC";
	#ifndef BIG_ENDIAN
	#define BIG_ENDIAN
	#endif
	#define CPU_PPC

	#if defined(__GNUC__)
		#define INLINE_ASM_STYLE_GCC_PPC
	#endif
#else
	#error "Unsupported CPU"
#endif
