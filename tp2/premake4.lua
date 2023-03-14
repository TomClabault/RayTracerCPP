dofile "gkit.lua"

 -- description des projets		 
projects = {
	"tp1",
}

for i, name in ipairs(projects) do
    project(name)
        language "C++"
        kind "ConsoleApp"
        targetdir "bin"
        files ( gkit_files )
        files { "projets/main.cpp" }
	files { "projets/triangle.cpp",
		"projets/ray.cpp",
		"projets/timer.cpp",
		"projets/objUtils.cpp" }
end
