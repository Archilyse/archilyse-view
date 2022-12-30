# Find Vulkan
#
# VULKAN_INCLUDE_DIR
# VULKAN_LIBRARY
# VULKAN_LIBRARY_DIR
# VULKAN_FOUND
# VULKANSDK_LAYERS_DIR
# VULKANSDK_FOUND

if (WIN32)
  find_path(VULKAN_INCLUDE_DIR NAMES vulkan/vulkan.h HINTS
    "$ENV{VULKAN_SDK}/Include"
    "$ENV{VK_SDK_PATH}/Include")

  if (CMAKE_CL_64)
  
    find_library(VULKAN_LIBRARY NAMES vulkan-1 HINTS
      "$ENV{VULKAN_SDK}/Bin"
      "$ENV{VK_SDK_PATH}/Bin"
      "$ENV{VULKAN_SDK}/Lib"
      "$ENV{VK_SDK_PATH}/Lib"
      )

    find_library(VULKAN_STATIC_LIBRARY NAMES vkstatic.1 HINTS
      "$ENV{VULKAN_SDK}/Bin"
      "$ENV{VK_SDK_PATH}/Bin"
      "$ENV{VULKAN_SDK}/Lib"
      "$ENV{VK_SDK_PATH}/Lib"
      )

    find_path(VULKAN_LIBRARY_DIR NAMES vulkan-1.lib vkstatic.1.lib HINTS
      "$ENV{VULKAN_SDK}/Bin"
      "$ENV{VK_SDK_PATH}/Bin"
      "$ENV{VULKAN_SDK}/Lib"
      "$ENV{VK_SDK_PATH}/Lib"
    )
  else()
    find_library(VULKAN_LIBRARY NAMES vulkan-1 HINTS
    "$ENV{VULKAN_SDK}/Bin32"
    "$ENV{VK_SDK_PATH}/Bin32"
    "$ENV{VULKAN_SDK}/Lib32"
    "$ENV{VK_SDK_PATH}/Lib32"
    )

  find_library(VULKAN_STATIC_LIBRARY NAMES vkstatic.1 HINTS
    "$ENV{VULKAN_SDK}/Bin32"
    "$ENV{VK_SDK_PATH}/Bin32"
    "$ENV{VULKAN_SDK}/Lib32"
    "$ENV{VK_SDK_PATH}/Lib32"
    )
  endif()

  find_path(VULKAN_LIBRARY_DIR NAMES vulkan-1.lib vkstatic.1.lib HINTS
    "$ENV{VULKAN_SDK}/Bin"
    "$ENV{VK_SDK_PATH}/Bin"
    "$ENV{VULKAN_SDK}/Lib"
    "$ENV{VK_SDK_PATH}/Lib"
  )
else()
  find_path(VULKAN_INCLUDE_DIR NAMES vulkan/vulkan.h HINTS
    "$ENV{VULKAN_SDK}/include"
    "/usr/local/x86_64/include")

  find_library(VULKAN_LIBRARY NAMES vulkan HINTS
    "$ENV{VULKAN_SDK}/lib"
    "/usr/local/x86_64/lib")
  
  find_path(VULKAN_LIBRARY_DIR NAMES libvulkan.so HINTS
    "$ENV{VULKAN_SDK}/lib"
    "/usr/local/x86_64/lib"
    "/usr/lib/x86_64-linux-gnu/")

  find_path(VULKANSDK_LAYERS_DIR NAMES VkLayer_core_validation.json PATHS
    "$ENV{VULKAN_SDK}/etc/explicit_layer.d"
    "/etc/vulkan/explicit_layer.d"
    "/usr/share/vulkan/explicit_layer.d"
    "~/.local/share/vulkan/explicit_layer.d"
    "/usr/local/x86_64/etc/explicit_layer.d")

  if (VULKANSDK_LAYERS_DIR)
    set (VULKANSDK_FOUND TRUE)
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Vulkan DEFAULT_MSG VULKAN_LIBRARY VULKAN_LIBRARY_DIR VULKAN_INCLUDE_DIR  )
