#pragma once
#include <stdio.h>

#ifdef PD_PLATFORM_WINDOWS
	#ifdef PD_BUILD_DLL
		#define PONDO_API __declspec(dllexport)
	#else
		#define PONDO_API __declspec(dllimport)
	#endif
#else
	#error Pondo only supports Windows!
#endif

#ifdef PD_DEBUG
	#define PD_CORE_ASSERT(x, ...) { if(!(x)) { __debugbreak(); } }
	#define PD_ASSERT(x, ...)      { if(!(x)) { __debugbreak(); } }
#else
	#define PD_CORE_ASSERT(x, ...)
	#define PD_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)
