#pragma once

#ifdef PD_PLATFORM_WINDOWS
	#ifdef PD_BUILD_DLL
		#define PONDO_API __declspec(dllexport)
	#else 
		#define PONDO_API __declspec(dllimport)
	#endif
#else
	#error Pondo only supports Windows!
#endif