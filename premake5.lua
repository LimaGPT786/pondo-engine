workspace "Pondo"
	architecture "x64"
	configurations { 
		"Debug", 
		"Release",
		"Dist"
	}

outputdir = "%{cfg.bulidcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["GLFW"] = "Pondo/vendor/GLFW/include"

include "Pondo/vendor/GLFW"

project "Pondo"
	location "Pondo"
	kind "SharedLib"
	language "C++"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	-- Everything in the src folder
	files {
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs {
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}"
	}

	links {
		"GLFW",
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

		-- PUt DLL in the same folder as the executable
		postbuildcommands {
			"{COPYFILE} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/Sandbox"
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
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	-- Everything in the src folder
	files {
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs {
		"Pondo/vendor/spdlog/include",
		"Pondo/src"
	}

	links {
		"Pondo"
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