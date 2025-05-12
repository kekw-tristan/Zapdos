workspace "GameProject"
    architecture "x64"
    startproject "System"

    configurations { "Debug", "Release" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Global Include directories
IncludeDir = {}
IncludeDir["DirectXTK12"] = "External/DirectXTK12/Inc"
IncludeDir["D3DX"]        = "External/d3dx"

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
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/src/**.hlsl"  -- Include shader files in project, but we won't compile them
    }

    includedirs {
        "%{prj.name}/src",
        IncludeDir["DirectXTK12"],
        IncludeDir["D3DX"]
    }

    links {
        "d3dcompiler"  -- Ensure you link d3dcompiler here as it's needed for compiling shaders at runtime
    }

    -- shaders are compiled during runtime, so no compilation in build step
    filter "files:**.hlsl"
        buildaction "None"  -- We don't want Premake to compile shaders at build time

    filter "configurations:Debug"
        symbols "On"

    filter "configurations:Release"
        optimize "On"

-- ================================
-- GUI Project
-- ================================
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

-- ================================
-- System Project (Main Executable)
-- ================================
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

    -- Post-build steps to copy shaders and resources to output directory
    postbuildcommands {
        -- Ensure the destination directory exists
        'if not exist "bin\\%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}\\Engine" mkdir "bin\\%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}\\Engine"',
        
        -- Copy the shaders
        'if exist "Engine\\src\\*.hlsl" xcopy /Q /Y /I "Engine\\src\\*.hlsl" "bin\\%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}\\Engine\\" > nul'
    }

    filter "configurations:Debug"
        symbols "On"

    filter "configurations:Release"
        optimize "On"
