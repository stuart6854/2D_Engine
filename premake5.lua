-- Include conan generate script
include('conanbuildinfo.premake.lua')

-- Main Workspace
workspace "2D Terrain"
    -- Import conan generate config
    conan_basic_setup()

    configurations { "Debug", "Release" }

    outputdir = "%{cfg.architecture}-%{cfg.system}-%{cfg.buildcfg}"

    include "App"