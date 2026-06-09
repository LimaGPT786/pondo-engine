workspace "Pondo"
	architecture "x64"
	configurations { 
		"Debug", 
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"]  = "Pondo/vendor/GLFW/include"
IncludeDir["glad"]  = "Pondo/vendor/glad/include"
IncludeDir["glm"]   = "Pondo/vendor/glm"
IncludeDir["ImGui"] = "Pondo/vendor/imgui"
IncludeDir["entt"]  = "Pondo/vendor/entt/include"

include "Pondo/vendor/GLFW"

-- glad static lib
project "glad"
	location "Pondo/vendor/glad"
	kind "StaticLib"
	language "C"
	staticruntime "On"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir    ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files { "Pondo/vendor/glad/src/glad.c" }

	includedirs { "Pondo/vendor/glad/include" }

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

-- ImGui static lib
project "ImGui"
	location "Pondo/vendor/imgui"
	kind "StaticLib"
	language "C++"
	staticruntime "On"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir    ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files {
		"Pondo/vendor/imgui/imgui.cpp",
		"Pondo/vendor/imgui/imgui_draw.cpp",
		"Pondo/vendor/imgui/imgui_tables.cpp",
		"Pondo/vendor/imgui/imgui_widgets.cpp",
		"Pondo/vendor/imgui/imgui_demo.cpp",
		"Pondo/vendor/imgui/imgui_impl_glfw.cpp",
		"Pondo/vendor/imgui/imgui_impl_opengl3.cpp"
	}

	includedirs {
		"Pondo/vendor/imgui",
		"Pondo/vendor/GLFW/include",
		"Pondo/vendor/glad/include"
	}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

project "Pondo"
	location "Pondo"
	kind "SharedLib"
	language "C++"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir    ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files {
		"Pondo/src/**.h",
		"Pondo/src/**.cpp"
	}

	includedirs {
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.glad}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.entt}"
	}

	links {
		"GLFW",
		"glad",
		"ImGui",
		"opengl32.lib"
	}

	filter "system:windows"
		cppdialect "C++23"
		staticruntime "On"
		systemversion "latest"

		defines {
			"PD_PLATFORM_WINDOWS",
			"PD_BUILD_DLL"
		}

		postbuildcommands {
			"{MKDIR} ../bin/" .. outputdir .. "/Sandbox",
			"{COPYFILE} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Sandbox/"
		}

	filter "configurations:Debug"
		defines "PD_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "PD_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "PD_DIST"
		optimize "On"

	filter "system:windows"
		buildoptions "/utf-8"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir    ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files {
		"Sandbox/src/**.h",
		"Sandbox/src/**.cpp"
	}

	includedirs {
		"Pondo/vendor/spdlog/include",
		"Pondo/src",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.glad}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.entt}"
	}

	links {
		"Pondo",
		"GLFW",
		"glad",
		"ImGui",
		"opengl32.lib"
	}

	filter "system:windows"
		cppdialect "C++23"
		staticruntime "On"
		systemversion "latest"

		defines {
			"PD_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines "PD_DEBUG"
		symbols "On"

	filter "configurations:Release"
		defines "PD_RELEASE"
		optimize "On"

	filter "configurations:Dist"
		defines "PD_DIST"
		optimize "On"

	filter "system:windows"
		buildoptions "/utf-8"