workspace "Zapdos"
    architecture "x64"
    startproject "Game"

    configurations { "Debug", "Release" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Global Include directories
IncludeDir = {}
IncludeDir["DirectXTK12"] = "External/DirectXTK12/Inc"
IncludeDir["D3DX"]        = "External/d3dx"
IncludeDir["tinygltf"]    = "External/tinygltf"
IncludeDir["DDSTextureLoader"] = "External/DirectXTex/DDSTextureLoader" 

-- ================================
-- Engine Project
-- ================================
project "Engine"
    location "Engine"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir    ("bin-int/" .. outputdir .. "/%{prj.name}")

    files {
        "Engine/src/**.h",
        "Engine/src/**.cpp",
        "External/DirectXTex/DDSTextureLoader/DDSTextureLoader12.cpp", 
    }

    includedirs {
        "Engine/src",
        IncludeDir["DirectXTK12"],
        IncludeDir["D3DX"],
        IncludeDir["tinygltf"],
        IncludeDir["DDSTextureLoader"], 
    }

    links {
        "d3dcompiler"
    }

    vpaths {
        ["Core/*"]      = "Engine/src/Core/**",
        ["Graphics/*"]  = "Engine/src/Graphics/**",
        ["Scene/*"]     = "Engine/src/Scene/**",
        ["Shaders/*"]   = "Assets/Shader/**",
    }

    -- Exclude HLSL files from compilation
    filter { "files:**.hlsl" }
        flags { "ExcludeFromBuild" }
    filter {}

    filter "configurations:Debug"
        symbols "On"

    filter "configurations:Release"
        optimize "On"

-- ================================
-- Game Project (Main Executable)
-- ================================
project "Game"
    location "Game"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir    ("bin-int/" .. outputdir .. "/%{prj.name}")

    files {
        "Game/src/**.h",
        "Game/src/**.cpp",
    }

    includedirs {
        "Game/src",
        "Engine/src",
        IncludeDir["DirectXTK12"],
        IncludeDir["D3DX"],
        IncludeDir["DDSTextureLoader"], 
    }

    links {
        "Engine",
        "d3d12",
        "dxgi",
        "d3dcompiler",
        "dxguid"
    }

    filter "configurations:Debug"
        symbols "On"

    filter "configurations:Release"
        optimize "On"