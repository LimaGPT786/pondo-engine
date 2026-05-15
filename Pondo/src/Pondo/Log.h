#pragma once

#include <memory>

#include "Core.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace Pondo {
	/*
		A static class for logging. It initializes the spdlog library 
		and provides a global logger instance that can be used throughout 
		the application.
	*/
	class PONDO_API Log {
	public:
		static void Init();
		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() {
			return s_coreLogger;
		};

		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() {
			return s_clientLogger;
		};

	private:
		static std::shared_ptr<spdlog::logger> s_coreLogger;
		static std::shared_ptr<spdlog::logger> s_clientLogger;

	};
}

// Core log macros
#define PD_CORE_TRACE(...)		::Pondo::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define PD_CORE_INFO(...)		::Pondo::Log::GetCoreLogger()->info(__VA_ARGS__)
#define PD_CORE_WARN(...)		::Pondo::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define PD_CORE_ERROR(...)		::Pondo::Log::GetCoreLogger()->error(__VA_ARGS__)
#define PD_CORE_CRITICAL(...)	::Pondo::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define PD_TRACE(...)		::Pondo::Log::GetClientLogger()->trace(__VA_ARGS__)
#define PD_INFO(...)		::Pondo::Log::GetClientLogger()->info(__VA_ARGS__)
#define PD_WARN(...)		::Pondo::Log::GetClientLogger()->warn(__VA_ARGS__)
#define PD_ERROR(...)		::Pondo::Log::GetClientLogger()->error(__VA_ARGS__)
#define PD_CRITICAL(...)	::Pondo::Log::GetClientLogger()->critical(__VA_ARGS__)