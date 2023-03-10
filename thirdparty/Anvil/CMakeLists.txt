cmake_minimum_required(VERSION 2.8)
project (Anvil)

option(ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT "Includes 32-/64-bit Windows window system support (Windows builds only)" ON)
option(ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT     "Includes XCB window system support (Linux builds only)" ON)
option(ANVIL_LINK_WITH_GLSLANG                     "Links with glslang, instead of spawning a new process whenever GLSL->SPIR-V conversion is required" ON)


# Do not modify anything after this line, unless you know what you're doing.
configure_file("include/config.h.in" "${Anvil_SOURCE_DIR}/include/config.h")

include_directories("$(Anvil_SOURCE_DIR)/deps"
                    "$(Anvil_SOURCE_DIR)/include")

if (WIN32)
    include_directories($ENV{VK_SDK_PATH}/Include
                        $ENV{VULKAN_SDK}/Include)
    
    if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
            link_directories   ($ENV{VK_SDK_PATH}/Bin
                                $ENV{VK_SDK_PATH}/Lib
                                $ENV{VULKAN_SDK}/Bin
                                $ENV{VULKAN_SDK}/Lib)
    else()
            link_directories   ($ENV{VK_SDK_PATH}/Bin32
                                $ENV{VK_SDK_PATH}/Lib32
                                $ENV{VULKAN_SDK}/Bin32
                                $ENV{VULKAN_SDK}/Lib32)
    endif()
else()
    include_directories($ENV{VK_SDK_PATH}/x86_64/include
                        $ENV{VULKAN_SDK}/include
                        $ENV{VULKAN_SDK}/x86_64/include)
    link_directories   ($ENV{VK_SDK_PATH}/x86_64/lib
                        $ENV{VULKAN_SDK}/lib
                        $ENV{VULKAN_SDK}/x86_64/lib)
endif()

SET (SRC_LIST "${Anvil_SOURCE_DIR}/include/misc/memalloc_backends/backend_oneshot.h"
              "${Anvil_SOURCE_DIR}/include/misc/memalloc_backends/backend_vma.h"
              "${Anvil_SOURCE_DIR}/include/misc/base_pipeline_manager.h"
              "${Anvil_SOURCE_DIR}/include/misc/callbacks.h"
              "${Anvil_SOURCE_DIR}/include/misc/debug.h"
              "${Anvil_SOURCE_DIR}/include/misc/debug_marker.h"
              "${Anvil_SOURCE_DIR}/include/misc/dummy_window.h"
              "${Anvil_SOURCE_DIR}/include/misc/formats.h"
              "${Anvil_SOURCE_DIR}/include/misc/fp16.h"
              "${Anvil_SOURCE_DIR}/include/misc/glsl_to_spirv.h"
              "${Anvil_SOURCE_DIR}/include/misc/io.h"
              "${Anvil_SOURCE_DIR}/include/misc/memory_allocator.h"
              "${Anvil_SOURCE_DIR}/include/misc/object_tracker.h"
              "${Anvil_SOURCE_DIR}/include/misc/page_tracker.h"
              "${Anvil_SOURCE_DIR}/include/misc/pools.h"
              "${Anvil_SOURCE_DIR}/include/misc/ref_counter.h"
              "${Anvil_SOURCE_DIR}/include/misc/shader_module_cache.h"
              "${Anvil_SOURCE_DIR}/include/misc/time.h"
              "${Anvil_SOURCE_DIR}/include/misc/types.h"
              "${Anvil_SOURCE_DIR}/include/misc/window.h"
              "${Anvil_SOURCE_DIR}/include/misc/window_factory.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/buffer.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/buffer_view.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/command_buffer.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/command_pool.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/compute_pipeline_manager.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/descriptor_pool.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/descriptor_set.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/descriptor_set_group.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/descriptor_set_layout.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/device.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/event.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/fence.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/framebuffer.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/graphics_pipeline_manager.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/image.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/image_view.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/instance.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/memory_block.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/physical_device.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/pipeline_cache.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/pipeline_layout.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/pipeline_layout_manager.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/query_pool.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/queue.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/render_pass.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/rendering_surface.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/sampler.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/semaphore.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/shader_module.h"
              "${Anvil_SOURCE_DIR}/include/wrappers/swapchain.h"

              "${Anvil_SOURCE_DIR}/src/misc/memalloc_backends/backend_oneshot.cpp"
              "${Anvil_SOURCE_DIR}/src/misc/memalloc_backends/backend_vma.cpp"
              "${Anvil_SOURCE_DIR}/src/misc/base_pipeline_manager.cpp"
              "${Anvil_SOURCE_DIR}/src/misc/debug.cpp"
              "${Anvil_SOURCE_DIR}/src/misc/debug_marker.cpp"
              "${Anvil_SOURCE_DIR}/src/misc/dummy_window.cpp"
              "${Anvil_SOURCE_DIR}/src/misc/formats.cpp"
              "${Anvil_SOURCE_DIR}/src/misc/fp16.cpp"
              "${Anvil_SOURCE_DIR}/src/misc/glsl_to_spirv.cpp"
              "${Anvil_SOURCE_DIR}/src/misc/io.cpp"
              "${Anvil_SOURCE_DIR}/src/misc/memory_allocator.cpp"
              "${Anvil_SOURCE_DIR}/src/misc/object_tracker.cpp"
              "${Anvil_SOURCE_DIR}/src/misc/page_tracker.cpp"
              "${Anvil_SOURCE_DIR}/src/misc/pools.cpp"
              "${Anvil_SOURCE_DIR}/src/misc/shader_module_cache.cpp"
              "${Anvil_SOURCE_DIR}/src/misc/time.cpp"
              "${Anvil_SOURCE_DIR}/src/misc/types.cpp"
              "${Anvil_SOURCE_DIR}/src/misc/window.cpp"
              "${Anvil_SOURCE_DIR}/src/misc/window_factory.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/buffer.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/buffer_view.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/command_buffer.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/command_pool.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/compute_pipeline_manager.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/descriptor_pool.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/descriptor_set.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/descriptor_set_group.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/descriptor_set_layout.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/device.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/event.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/fence.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/framebuffer.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/graphics_pipeline_manager.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/image.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/image_view.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/instance.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/memory_block.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/physical_device.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/pipeline_cache.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/pipeline_layout.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/pipeline_layout_manager.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/query_pool.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/queue.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/render_pass.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/rendering_surface.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/sampler.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/semaphore.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/shader_module.cpp"
              "${Anvil_SOURCE_DIR}/src/wrappers/swapchain.cpp"

# Vulkan Memory Allocator dependency
              "${Anvil_SOURCE_DIR}/deps/VulkanMemoryAllocator/vk_mem_alloc.h"
                         )

# prepare source code files for different OS
if(WIN32)
    if (ANVIL_INCLUDE_WIN3264_WINDOW_SYSTEM_SUPPORT)
        list (APPEND SRC_LIST "${Anvil_SOURCE_DIR}/include/misc/window_win3264.h"
                              "${Anvil_SOURCE_DIR}/src/misc/window_win3264.cpp")
    endif()
else()
    if (ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT)
        list (APPEND SRC_LIST
                              "${Anvil_SOURCE_DIR}/include/misc/xcb_loader.h"
                              "${Anvil_SOURCE_DIR}/src/misc/xcb_loader.cpp"
                              "${Anvil_SOURCE_DIR}/include/misc/window_xcb.h"
                              "${Anvil_SOURCE_DIR}/src/misc/window_xcb.cpp")
    endif()
endif()

add_library(Anvil STATIC ${SRC_LIST})

if (WIN32)
    if (ANVIL_LINK_WITH_GLSLANG)
        target_link_libraries(Anvil glslang HLSL OGLCompiler OSDependent SPIRV vulkan-1.lib)
    else()
        target_link_libraries(Anvil vulkan-1.lib)
    endif()
else()
    if (ANVIL_LINK_WITH_GLSLANG)
        target_link_libraries(Anvil glslang HLSL OGLCompiler OSDependent SPIRV vulkan pthread)
    else()
        target_link_libraries(Anvil vulkan pthread)
    endif()
endif()

if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -std=c++11")
else()
    set(CMAKE_CXX_FLAGS "$(CMAKE_CXX_FLAGS) /EHsc /MP")
endif()

# Add dependendencies
if (WIN32)
    add_subdirectory("deps\\glslang")
else()
    add_subdirectory("deps/glslang")
endif()

# Enable level-4 warnings
if (MSVC)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

if (CMAKE_COMPILER_IS_GNUCC)
    target_compile_options(Anvil PRIVATE "-Wall")
endif (CMAKE_COMPILER_IS_GNUCC)
if (MSVC)
    target_compile_options(Anvil PRIVATE "/W4" "/WX" "/wd4191" "/wd4350" "/wd4668" "/wd4820" "/wd4514" "/wd4061" "/wd4710" "/wd4711" "/wd4548" "/wd4555")
endif (MSVC)

# Add all demo sub-projects
MESSAGE(STATUS "Using Vulkan SDK at either " $ENV{VK_SDK_PATH} " or " $ENV{VULKAN_SDK})
