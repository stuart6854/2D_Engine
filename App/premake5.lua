project "App"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    targetname "app"
    targetdir("../bin/" .. outputdir)
    objdir("../bin/" .. outputdir .. "/obj/%{prj.name}")
    debugdir ("../bin/" .. outputdir)
    staticruntime "Off"

    flags
    {
        "MultiProcessorCompile",
        "FatalCompileWarnings",
    }
    warnings "High"
    
    linkoptions { conan_exelinkflags }
    
    files { "src/**.hpp", "src/**.h", "src/**.cpp" }
    includedirs { "src" }

    externalwarnings "Off"
    externalanglebrackets "On"
    externalincludedirs { "$(VULKAN_SDK)/Include" }

    links { 
        "$(VULKAN_SDK)/Lib/vulkan-1.lib",
    }

    defines
    {
        "GLFW_INCLUDE_NONE",
        "VK_USE_PLATFORM_WIN32_KHR",
    }

    filter "configurations:Debug"
        targetsuffix "-debug"
        defines { "_DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"