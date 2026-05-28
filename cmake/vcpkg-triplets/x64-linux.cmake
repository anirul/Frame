set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

set(_frame_homebrew_prefix "$ENV{HOMEBREW_PREFIX}")
if(NOT _frame_homebrew_prefix)
    find_program(_frame_homebrew_command NAMES brew)
    if(_frame_homebrew_command)
        execute_process(
            COMMAND "${_frame_homebrew_command}" --prefix
            OUTPUT_VARIABLE _frame_homebrew_prefix
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
    endif()
endif()

function(_frame_prepend_pkg_config_path path)
    if(NOT EXISTS "${path}")
        return()
    endif()

    if(DEFINED ENV{PKG_CONFIG_PATH} AND NOT "$ENV{PKG_CONFIG_PATH}" STREQUAL "")
        string(REPLACE ":" ";" _frame_pkg_config_path_list "$ENV{PKG_CONFIG_PATH}")
        list(FIND _frame_pkg_config_path_list "${path}" _frame_pkg_config_path_index)
        if(_frame_pkg_config_path_index EQUAL -1)
            set(ENV{PKG_CONFIG_PATH} "${path}:$ENV{PKG_CONFIG_PATH}")
        endif()
    else()
        set(ENV{PKG_CONFIG_PATH} "${path}")
    endif()
endfunction()

if(_frame_homebrew_prefix AND EXISTS "${_frame_homebrew_prefix}")
    foreach(_frame_pkgconfig_dir IN ITEMS
        "${_frame_homebrew_prefix}/lib/pkgconfig"
        "${_frame_homebrew_prefix}/share/pkgconfig")
        _frame_prepend_pkg_config_path("${_frame_pkgconfig_dir}")
    endforeach()

    file(GLOB _frame_homebrew_opt_pkgconfig_dirs
        LIST_DIRECTORIES true
        "${_frame_homebrew_prefix}/opt/*/lib/pkgconfig"
        "${_frame_homebrew_prefix}/opt/*/share/pkgconfig")
    list(SORT _frame_homebrew_opt_pkgconfig_dirs)
    foreach(_frame_pkgconfig_dir IN LISTS _frame_homebrew_opt_pkgconfig_dirs)
        _frame_prepend_pkg_config_path("${_frame_pkgconfig_dir}")
    endforeach()
endif()

set(VCPKG_C_FLAGS "")
set(VCPKG_CXX_FLAGS "")

unset(_frame_homebrew_command)
unset(_frame_homebrew_opt_pkgconfig_dirs)
unset(_frame_homebrew_prefix)
unset(_frame_pkgconfig_dir)
