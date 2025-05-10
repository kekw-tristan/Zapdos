workspace "GameProject"
    architecture "x64"
    startproject "System"

    configurations { "Debug", "Release" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Global Include directories
IncludeDir = {}
IncludeDir["DirectXTK12"] = "External/DirectXTK12/Inc"
IncludeDir["D3DX"]        = "External/d3dx"

-- =============================
-- Engine Project
-- =============================
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
        "%{prj.name}/src/**.cpp",
        -- "%{prj.name}/src/**.hlsl"
    }

    includedirs {
        "%{prj.name}/src",
        IncludeDir["DirectXTK12"],
        IncludeDir["D3DX"]
    }

    filter "configurations:Debug"
        symbols "On"

    filter "configurations:Release"
        optimize "On"

-- =============================
-- GUI Project
-- =============================
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
        IncludeDir["D3DX"]
    }

    filter "configurations:Debug"
        symbols "On"

    filter "configurations:Release"
        optimize "On"

-- =============================
-- System Project (Main Executable)
-- =============================
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
        IncludeDir["D3DX"]
    }

    links {
        "Engine",
        "GUI",
        "d3d12",
        "dxgi",
        "d3dcompiler",
        "dxguid"
    }

    filter "configurations:Debug"
        symbols "On"

    filter "configurations:Release"
        optimize "On"
