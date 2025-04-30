workspace "GameProject"
    architecture "x64"
    configurations { "Debug", "Release" }
    startproject "System"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Globale Includes
IncludeDir = {}
IncludeDir["DirectXTK12"] = "External/DirectXTK12/Inc"
IncludeDir["D3DX"] = "External/d3dx"  -- Füge d3dx als Include hinzu

-- Engine
project "Engine"
    location "Engine"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir    ("bin-int/" .. outputdir .. "/%{prj.name}")

    files {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp"
    }

    includedirs {
        "%{prj.name}/src",
        IncludeDir["DirectXTK12"],
        IncludeDir["D3DX"]  -- Füge den d3dx Include hier hinzu
    }

    filter "configurations:Debug"
        symbols "On"

    filter "configurations:Release"
        optimize "On"

-- GUI
project "GUI"
    location "GUI"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir    ("bin-int/" .. outputdir .. "/%{prj.name}")

    files {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp"
    }

    includedirs {
        "%{prj.name}/src",
        IncludeDir["DirectXTK12"],
        IncludeDir["D3DX"]  -- Füge den d3dx Include hier hinzu
    }

    filter "configurations:Debug"
        symbols "On"

    filter "configurations:Release"
        optimize "On"

-- System (main app)
project "System"
    location "System"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir    ("bin-int/" .. outputdir .. "/%{prj.name}")

    files {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp"
    }

    includedirs {
        "System/src",
        "Engine/src",
        "GUI/src",
        IncludeDir["DirectXTK12"],
        IncludeDir["D3DX"]  -- Füge den d3dx Include hier hinzu
    }

    links {
        "Engine",
        "GUI"
    }

    -- Windows system libs required for DirectX 12
    links {
        "d3d12",
        "dxgi",
        "d3dcompiler"
    }

    filter "configurations:Debug"
        symbols "On"

    filter "configurations:Release"
        optimize "On"
