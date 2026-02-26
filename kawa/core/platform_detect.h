#ifndef KAWA_PLATFORM_DETECT
#define KAWA_PLATFORM_DETECT

#if defined(__clang__)
#	define kw_compiler_clang
#elif defined(_MSC_VER)
#	define kw_compiler_msvc
#elif defined(__GNUC__) 
#	define kw_compiler_gnu
#else
#	define kw_compiler_unknown
#endif

#if defined(_WIN32)
#	define kw_platform_windows
#elif defined(__linux__)
#	define kw_platform_linux
#elif defined(__APPLE__) 
	#define kw_platform_apple
	// __APPLE__ is defined for variety of apple devices, so we need to narrow down to specifically macos
	#include <TargetConditionals.h>
	#if TARGET_OS_OSX
		#define kw_platform_macos
	#endif
#else
#define kw_platform_unknown
#endif

#ifdef kw_platform_windows
	#define kw_platform windows
#endif

#ifdef kw_platform_linux
#define kw_platform linux
#endif

#ifdef kw_platform_macos
	#define kw_platform macOS
#endif

#ifdef kw_platform_unknown
	#define kw_platform unknown
#endif

#endif