set(VCPKG_LIBRARY_LINKAGE dynamic)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO KhronosGroup/Vulkan-Loader
    REF "vulkan-sdk-${VERSION}"
    SHA512 974da8a010ed41eec465ef964f7c944f1d10c8a73f6b234da5b0ea341ff805de6bfa88643b2debb88f4d95752d9df144066cb5e10fcb0f9b1b5c0c7e233e0aad
    HEAD_REF main
)

vcpkg_find_acquire_program(PYTHON3)

set(_frame_vulkan_loader_options "")
if(VCPKG_TARGET_IS_LINUX)
    list(APPEND _frame_vulkan_loader_options
        -DBUILD_WSI_XCB_SUPPORT:BOOL=OFF
        -DBUILD_WSI_XLIB_SUPPORT:BOOL=OFF
        -DBUILD_WSI_XLIB_XRANDR_SUPPORT:BOOL=OFF
        -DBUILD_WSI_WAYLAND_SUPPORT:BOOL=ON)
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        ${_frame_vulkan_loader_options}
        -DBUILD_TESTS:BOOL=OFF
        -DPython3_EXECUTABLE=${PYTHON3}
    MAYBE_UNUSED_VARIABLES
        BUILD_WSI_XLIB_XRANDR_SUPPORT
        Python3_EXECUTABLE
)
vcpkg_cmake_install()
vcpkg_fixup_pkgconfig()
vcpkg_cmake_config_fixup(CONFIG_PATH "lib/cmake/VulkanLoader" PACKAGE_NAME VulkanLoader)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

configure_file("${CMAKE_CURRENT_LIST_DIR}/usage" "${CURRENT_PACKAGES_DIR}/share/${PORT}/usage" @ONLY)

unset(_frame_vulkan_loader_options)
