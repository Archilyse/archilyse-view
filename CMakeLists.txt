cmake_minimum_required(VERSION 3.5)

#compiler options
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")


# setup project
# Project
project(archilyse-view)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 4)
set(VERSION_PATCH 0)
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set(EXEC_NAME "quavis")

# find dependencies
find_package (Vulkan REQUIRED)


# Compiler
if (WIN32)
    add_definitions (-D_CRT_SECURE_NO_WARNINGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
else (WIN32)
    add_definitions ("-g")
    add_definitions ("-O3")
    add_definitions ("-Wno-maybe-uninitialized") # required for O3
    add_definitions ("-Wno-missing-braces") #because of Anvil
endif (WIN32)

# Compile thirdparty
# Anvil
set(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT OFF CACHE BOOL "Includes 32-/64-bit Windows window system support (Windows builds only)")
set(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT OFF CACHE BOOL "Includes XCB window system support (Linux builds only)")
set(ANVIL_LINK_WITH_GLSLANG  ON CACHE BOOL "Links with glslang, instead of spawning a new process whenever GLSL->SPIR-V conversion is required")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/Anvil")

include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/Anvil/include")
include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/json/include")
include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/spdlog/include")
include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/stb/include")
include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glm")

 
# SET(SOURCES 
#   "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
# )

# while develop we glob later we add exlicitly
FILE(GLOB_RECURSE SOURCES
"src/*.h"
"src/*.cpp"
)

# evil hack because anvil includes without directory
message(${VULKAN_LIBRARY_DIR})
link_directories( ${VULKAN_LIBRARY_DIR})

add_executable(${EXEC_NAME} "${SOURCES}")

# for visual studio to get "folders"
function(assign_source_group)
foreach(_source IN ITEMS ${ARGN})
    if (IS_ABSOLUTE "${_source}")
        file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
    else()
        set(_source_rel "${_source}")
    endif()
    get_filename_component(_source_path "${_source_rel}" PATH)
    string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
    source_group("${_source_path_msvc}" FILES "${_source}")
endforeach()
endfunction(assign_source_group)

assign_source_group(${SOURCES})

TARGET_LINK_LIBRARIES(${EXEC_NAME} 
    Anvil 
    ${VULKAN_LIBRARY}
)




# link needed things
if (WIN32)
# TODO: Compile shaders in CMake under windows
else (WIN32)
  TARGET_LINK_LIBRARIES( ${EXEC_NAME}
  pthread
  stdc++fs
  )
endif (WIN32)


target_link_libraries(${EXEC_NAME} -static-libgcc -static-libstdc++)

# set_target_properties(${EXEC_NAME} PROPERTIES LINKER_LANGUAGE CXX)

# Installation
set (CMAKE_INSTALL_PREFIX "/usr/")
install(TARGETS ${EXEC_NAME} DESTINATION bin COMPONENT view)

# Packaging
include (InstallRequiredSystemLibraries)
set (CPACK_PACKAGE_CONTACT "franzen@archilyse.com")
set (CPACK_PACKAGE_MAINTAINER "Michael Franzen <franzen@archilyse.com>")
set (CPACK_PACKAGE_NAME "ArchilyseView")
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "Binary for running view simulations" )

set (CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
set (CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}")
set (CPACK_PACKAGE_VERSION "${VERSION}")
set (CPACK_NSIS_MODIFY_PATH "ON")

set (CPACK_GENERATOR "DEB")

include (CPack)

