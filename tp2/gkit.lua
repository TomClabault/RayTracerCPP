solution "gKit3"
    configurations { "debug", "release" }
    
    includedirs { ".", "src" }
    
    configuration "debug"
        targetdir "bin/debug"
        defines { "DEBUG" }
        if _PREMAKE_VERSION >="5.0" then
            symbols "on"
        else
            flags { "Symbols" }
        end
    
    configuration "release"
        targetdir "bin/release"
--~ 		defines { "NDEBUG" }
--~ 		defines { "GK_RELEASE" }
        if _PREMAKE_VERSION >="5.0" then
            optimize "speed"
        else
            flags { "OptimizeSpeed" }
        end
        
    configuration "linux"
        buildoptions { "-mtune=native -march=native" }
        buildoptions { "-std=c++11" }
        buildoptions { "-W -Wall -Wextra -Wsign-compare -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable", "-pipe" }
    
    configuration { "linux", "debug" }
        buildoptions { "-g"}
        linkoptions { "-g"}
    
    configuration { "linux", "release" }
        buildoptions { "-fopenmp" }
        linkoptions { "-fopenmp" }
        buildoptions { "-flto"}
        linkoptions { "-flto"}
    
if _PREMAKE_VERSION >="5.0" then
    configuration { "windows" }
        location "build"
        debugdir "."
        
        defines { "WIN32", "_USE_MATH_DEFINES", "_CRT_SECURE_NO_WARNINGS" }
        defines { "NOMINMAX" } -- allow std::min() and std::max() in vc++ :(((

    configuration { "windows", "vs*" }
        location "build"
        debugdir "."
        
        system "Windows"
        architecture "x64"
        disablewarnings { "4244", "4305" }
        flags { "MultiProcessorCompile", "NoMinimalRebuild" }
        
--~         includedirs { "extern/visual/include" }
--~         libdirs { "extern/visual/lib" }
--~         links { "opengl32", "glew32", "SDL2", "SDL2main", "SDL2_image" }
end
    
    configuration "macosx"
        frameworks= "-F /Library/Frameworks/"
        buildoptions { "-std=c++14 -Wno-deprecated-declarations" }
--~         defines { "GK_MACOS" }
        buildoptions { frameworks }
--~         linkoptions { frameworks .. " -framework OpenGL -framework SDL2 -framework SDL2_image" }
    
 -- description des fichiers communs
gkit_files = { "src/*.cpp", "src/*.h" }


