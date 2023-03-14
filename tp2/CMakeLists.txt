cmake_minimum_required(VERSION 3.5)

project(RayTracing_TP1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Defining the list of the header files
file(GLOB INCLUDE_FILES_SRC ${PROJECT_SOURCE_DIR}/src/*.h)
file(GLOB INCLUDE_FILES_PROJETS ${PROJECT_SOURCE_DIR}/projets/*.h)
file(GLOB INCLUDE_FILES_PROJETS_SIMD ${PROJECT_SOURCE_DIR}/projets/SIMD/*.h)
list(APPEND INCLUDE_FILES ${INCLUDE_FILES_SRC})
list(APPEND INCLUDE_FILES ${INCLUDE_FILES_PROJETS})
list(APPEND INCLUDE_FILES ${INCLUDE_FILES_PROJETS_SIMD})

#Defining the variables SRC_FILES and INCLUDE_FILES as the list of all recursively found cpp files and header files in the PROJECT_SOURCE_DIR
file(GLOB SRC_FILES_SRC ${PROJECT_SOURCE_DIR}/src/*.cpp)
file(GLOB SRC_FILES_PROJETS ${PROJECT_SOURCE_DIR}/projets/*.cpp)
file(GLOB SRC_FILES_PROJETS_SIMD ${PROJECT_SOURCE_DIR}/projets/SIMD/*.cpp)
list(APPEND SRC_FILES ${SRC_FILES_SRC})
list(APPEND SRC_FILES ${SRC_FILES_PROJETS})
list(APPEND SRC_FILES ${SRC_FILES_PROJETS_SIMD})

#Copying ${SRC_FILES} into ${SRC_FILES_TESTS}
list(APPEND SRC_FILES_TESTS ${SRC_FILES})

list(REMOVE_ITEM SRC_FILES_TESTS ${PROJECT_SOURCE_DIR}/projets/main.cpp)
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/projets/tests.cpp)
list(APPEND SRC_FILES ${INCLUDE_FILES})
list(APPEND SRC_FILES_TESTS ${INCLUDE_FILES})



add_executable(RayTracing_TP1 ${SRC_FILES})
add_executable(RayTracing_TP1_Tests ${SRC_FILES_TESTS})

target_include_directories(RayTracing_TP1 PRIVATE src/)
target_include_directories(RayTracing_TP1 PRIVATE projets/)
target_include_directories(RayTracing_TP1 PRIVATE projets/SIMD/)

target_include_directories(RayTracing_TP1_Tests PRIVATE src/)
target_include_directories(RayTracing_TP1_Tests PRIVATE projets/)
target_include_directories(RayTracing_TP1_Tests PRIVATE projets/SIMD/)

#Adding OpenMP to the projects
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(RayTracing_TP1_Tests PUBLIC OpenMP::OpenMP_CXX)
    target_link_libraries(RayTracing_TP1 PUBLIC OpenMP::OpenMP_CXX)
endif()

#Copying the "data" folder into the binary directory so that the application has access to the data folder even when in the folders created by the QtCreator "compiler"
file(COPY data DESTINATION ${PROJECT_BINARY_DIR})
message("/data OBJ files copied")

#If CMAKE_BUILD_TYPE hasn't been specified on the commandline, defaulting to release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	message(GNU Compiler Detected)

	set(CMAKE_CXX_FLAGS "-Wall -Wextra") #Flags always active
	set(CMAKE_CXX_FLAGS_DEBUG "-g") #Flags when compiling in debug
	set(CMAKE_CXX_FLAGS_RELEASE "-O3") #Flags when compiling in release
	
	target_compile_options(RayTracing_TP1 PRIVATE -fopenmp)
	target_compile_options(RayTracing_TP1_Tests PRIVATE -fopenmp)
endif()
