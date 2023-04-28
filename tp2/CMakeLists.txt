cmake_minimum_required(VERSION 3.5)

project(RayTracing LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

#Defining the list of the header files
file(GLOB INCLUDE_FILES_SRC ${PROJECT_SOURCE_DIR}/src/*.h)
file(GLOB INCLUDE_FILES_PROJETS ${PROJECT_SOURCE_DIR}/projets/*.h)
file(GLOB INCLUDE_FILES_PROJETS_SIMD ${PROJECT_SOURCE_DIR}/projets/SIMD/*.h)
file(GLOB INCLUDE_FILES_PROJETS_UTILS ${PROJECT_SOURCE_DIR}/projets/utils/*.h)
file(GLOB INCLUDE_FILES_PROJETS_SCENE ${PROJECT_SOURCE_DIR}/projets/scene/*.h)
file(GLOB INCLUDE_FILES_PROJETS_RENDERER ${PROJECT_SOURCE_DIR}/projets/renderer/*.h)
file(GLOB INCLUDE_FILES_PROJETS_QT ${PROJECT_SOURCE_DIR}/projets/QT/*.h)
list(APPEND INCLUDE_FILES ${INCLUDE_FILES_SRC})
list(APPEND INCLUDE_FILES ${INCLUDE_FILES_PROJETS})
list(APPEND INCLUDE_FILES ${INCLUDE_FILES_PROJETS_SIMD})
list(APPEND INCLUDE_FILES ${INCLUDE_FILES_PROJETS_UTILS})
list(APPEND INCLUDE_FILES ${INCLUDE_FILES_PROJETS_SCENE})
list(APPEND INCLUDE_FILES ${INCLUDE_FILES_PROJETS_RENDERER})
list(APPEND INCLUDE_FILES ${INCLUDE_FILES_PROJETS_QT})

#Defining the source files
file(GLOB SRC_FILES_SRC ${PROJECT_SOURCE_DIR}/src/*.cpp)
file(GLOB SRC_FILES_PROJETS ${PROJECT_SOURCE_DIR}/projets/*.cpp)
file(GLOB SRC_FILES_PROJETS_SIMD ${PROJECT_SOURCE_DIR}/projets/SIMD/*.cpp)
file(GLOB SRC_FILES_PROJETS_UTILS ${PROJECT_SOURCE_DIR}/projets/utils/*.cpp)
file(GLOB SRC_FILES_PROJETS_SCENE ${PROJECT_SOURCE_DIR}/projets/scene/*.cpp)
file(GLOB SRC_FILES_PROJETS_RENDERER ${PROJECT_SOURCE_DIR}/projets/renderer/*.cpp)
file(GLOB SRC_FILES_PROJETS_QT ${PROJECT_SOURCE_DIR}/projets/QT/*.cpp)
list(APPEND SRC_FILES ${SRC_FILES_SRC})
list(APPEND SRC_FILES ${SRC_FILES_PROJETS})
list(APPEND SRC_FILES ${SRC_FILES_PROJETS_SIMD})
list(APPEND SRC_FILES ${SRC_FILES_PROJETS_UTILS})
list(APPEND SRC_FILES ${SRC_FILES_PROJETS_SCENE})
list(APPEND SRC_FILES ${SRC_FILES_PROJETS_RENDERER})
list(APPEND SRC_FILES ${SRC_FILES_PROJETS_QT})

file(GLOB UI_FILES_PROJETS_QT ${PROJECT_SOURCE_DIR}/projets/QT/*.ui)
#file(GLOB QRC_FILES_PROJETS_QT ${PROJECT_SOURCE_DIR}/darkstyle/*.qrc)

#Copying ${SRC_FILES} into ${SRC_FILES_TESTS}
list(APPEND SRC_FILES_TESTS ${SRC_FILES})

list(REMOVE_ITEM SRC_FILES_TESTS ${PROJECT_SOURCE_DIR}/projets/main.cpp)
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/projets/tests.cpp)
list(APPEND SRC_FILES ${INCLUDE_FILES})
list(APPEND SRC_FILES ${UI_FILES_PROJETS_QT})
list(APPEND SRC_FILES ${QRC_FILES_PROJETS_QT})
list(APPEND SRC_FILES_TESTS ${INCLUDE_FILES})
list(APPEND SRC_FILES_TESTS ${UI_FILES_PROJETS_QT})

add_executable(RayTracing ${SRC_FILES})
add_executable(RayTracing_Tests ${SRC_FILES_TESTS})

target_include_directories(RayTracing PRIVATE src/)
target_include_directories(RayTracing PRIVATE projets/)
target_include_directories(RayTracing PRIVATE projets/SIMD/)
target_include_directories(RayTracing PRIVATE projets/utils/)
target_include_directories(RayTracing PRIVATE projets/scene/)
target_include_directories(RayTracing PRIVATE projets/renderer/)
target_include_directories(RayTracing PRIVATE projets/QT/)

target_include_directories(RayTracing_Tests PRIVATE src/)
target_include_directories(RayTracing_Tests PRIVATE projets/)
target_include_directories(RayTracing_Tests PRIVATE projets/SIMD/)
target_include_directories(RayTracing_Tests PRIVATE projets/utils/)
target_include_directories(RayTracing_Tests PRIVATE projets/scene/)
target_include_directories(RayTracing_Tests PRIVATE projets/renderer/)
target_include_directories(RayTracing_Tests PRIVATE projets/QT/)

#Adding OpenMP to the projects
find_package(OpenMP)

set (CMAKE_PREFIX_PATH "E:\\Programmes\\Qt\\6.3.2\\msvc2019_64")
find_package(Qt6 COMPONENTS Core Widgets Gui OpenGL OpenGLWidgets)
if (Qt6_FOUND)
    target_link_libraries(RayTracing PUBLIC OpenMP::OpenMP_CXX Qt6::Core Qt6::Widgets Qt6::Gui)
    target_link_libraries(RayTracing_Tests PUBLIC OpenMP::OpenMP_CXX Qt6::Core Qt6::Widgets Qt6::Gui)

    qt_standard_project_setup()
else()
    find_package(Qt5 REQUIRED COMPONENTS Widgets)
    if (Qt5Widgets_FOUND)
        message ("Qt5 Found!")
        target_link_libraries(RayTracing PUBLIC OpenMP::OpenMP_CXX Qt5::Core Qt5::Widgets Qt5::Gui)
        target_link_libraries(RayTracing_Tests PUBLIC OpenMP::OpenMP_CXX Qt5::Core Qt5::Widgets Qt5::Gui)
    endif()
endif()


#Copying the "data" folder into the binary directory so that the application has access to the data folder even when in the folders created by the QtCreator "compiler"
message(${PROJECT_BINARY_DIR})
file(COPY data DESTINATION ${PROJECT_BINARY_DIR})
file(COPY darkstyle DESTINATION ${PROJECT_BINARY_DIR})
message("/data OBJ files copied")

#If CMAKE_BUILD_TYPE hasn't been specified on the commandline, defaulting to release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message("GNU Compiler Detected")

    set(CMAKE_CXX_FLAGS "-Wall -Wextra -march=native -mfma") #Flags always active
    set(CMAKE_CXX_FLAGS_DEBUG "-g") #Flags when compiling in debug
    set(CMAKE_CXX_FLAGS_RELEASE "-O3") #Flags when compiling in release

    target_compile_options(RayTracing PRIVATE -fopenmp)
    target_compile_options(RayTracing_Tests PRIVATE -fopenmp)

    if (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g") #Flags when compiling in release
    endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message ("MSVC Compiler Detected")

    target_compile_options(RayTracing PRIVATE /arch:AVX2)

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS_RELEASE "/Od /g")#Flags when compiling in debug.
    elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set(CMAKE_CXX_FLAGS_RELEASE "/g") #Flags when compiling in release with debug info
    elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_CXX_FLAGS_RELEASE "/Ox") #Flags when compiling in release.
        #We need to specify both options at the same time as another call to "set()"
        #would overwrite the previous one
    endif()
endif()

if (WIN32)
    # find qmake executable
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)

    # run windeployqt
    add_custom_command(
        TARGET RayTracing POST_BUILD
        COMMAND "${_qt_bin_dir}/windeployqt.exe" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/RayTracing.exe
    )
endif()
