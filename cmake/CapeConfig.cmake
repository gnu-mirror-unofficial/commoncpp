#.rst:
# CapeConfig
# ---------------

#=============================================================================
# Copyright (C) 2015 David Sugar, Tycho Softworks.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#=============================================================================

include(GNUInstallDirs)
include(CheckFunctionExists)
include(CheckIncludeFiles)

macro(check_functions)
    foreach(arg ${ARGN})
        string(TOUPPER "${arg}" _fn)
        set(_fn "HAVE_${_fn}")
        check_function_exists(${arg} ${_fn})
    endforeach()
endmacro()

macro(check_headers)
    foreach(arg ${ARGN})
        string(TOUPPER "${arg}" _hdr)
        string(REGEX REPLACE "/" "_" _hdr ${_hdr})
        string(REGEX REPLACE "[.]" "_" _hdr ${_hdr})
        set(_hdr "HAVE_${_hdr}")
        check_include_files(${arg} ${_hdr})
    endforeach()
endmacro()

macro(target_setuid_properties)
    if(CMAKE_COMPILER_IS_GNUCXX AND NOT CMAKE_COMPILE_SETUID_FLAGS)
	    set(CMAKE_COMPILE_SETUID_FLAGS "-O2 -fPIE -fstack-protector -D_FORTIFY_SOURCE=2 --param ssp-buffer-size=4 -pie") 
        set(CMAKE_LINK_SETUID_FLAGS "-pie -z relro -z now")
    endif()
    foreach(arg ${ARGN})
        set_target_properties(${arg} PROPERTIES COMPILE_FLAGS "${CMAKE_COMPILE_SETUID_FLAGS}" LINK_FLAGS "${CMAKE_LINK_SETUID_FLAGS}" POSITION_INDEPENDENT_CODE TRUE )
    endforeach()
endmacro()

function(add_library_version _LIBRARY)
    set(_VERSION "${ARGN}")
    set(_SOVERSION "-")
    if(UNIX AND "${_VERSION}" STREQUAL "" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake-abi.sh")
        execute_process(WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMMAND "./cmake-abi.sh" OUTPUT_VARIABLE _VERSION)
    endif()
	if(NOT UNIX OR "${_VERSION}" STREQUAL "")
		set(_VERSION} "${VERSION}")
	endif()
	STRING(REGEX REPLACE "[.].*$" "" _SOVERSION ${_VERSION})
	if(ABI_MAJORONLY)
		set(_VERSION ${_SOVERSION})
	endif()
    set_target_properties(${_LIBRARY} PROPERTIES VERSION ${_VERSION} SOVERSION ${_SOVERSION})
endfunction()

function(create_specfile)
    if(ARGN)
        foreach(arg ${ARGN})
            configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${arg}.spec.cmake ${CMAKE_CURRENT_SOURCE_DIR}/${arg}.spec @ONLY)
        endforeach()
    else()
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.spec.cmake ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.spec @ONLY)
    endif()
endfunction()

function(create_rcfiles _OUTPUT)
    set(RC_VERSION ${VERSION})
	string(REGEX REPLACE "[.]" "," RC_VERSION ${VERSION})
	set(RC_VERSION "${RC_VERSION},0")
    set(RC_FILES)

    if(ARGN)
        foreach(arg ${ARGN})
            configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${arg}.rc.cmake ${CMAKE_CURRENT_SOURCE_DIR}/${arg}.rc)
            set(RC_FILES ${RC_FILES} ${arg}.rc)
        endforeach()
    else()
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.rc.cmake ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.rc)
        set(RC_FILES ${PROJECT_NAME}.rc)
    endif()
    if(WIN32)
        set(${_OUTPUT} ${RC_FILES} PARENT_SCOPE)
    else()
        set(${_OUTPUT} PARENT_SCOPE)
    endif()
endfunction()

function(create_scripts _OUTPUT)
    set(SCR_FILES)
    foreach(arg ${ARGN})
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${arg}.cmake ${CMAKE_CURRENT_BINARY_DIR}/${arg})
        set(SCR_FILES ${SCR_FILES} ${CMAKE_CURRENT_BINARY_DIR}/${arg})
    endforeach()
    set(${_OUTPUT} ${SCR_FILES} PARENT_SCOPE)
endfunction()

function(create_pcfiles _OUTPUT)
    set(PC_FILES)
    if(ARGN)
        foreach(arg ${ARGN})
            configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${arg}.pc.cmake ${CMAKE_CURRENT_BINARY_DIR}/${arg}.pc)
            set(PC_FILES ${PC_FILES} ${CMAKE_CURRENT_BINARY_DIR}/${arg}.pc)
        endforeach()
    else()
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.pc.cmake ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc)
        set(PC_FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc)
    endif()
    set(${_OUTPUT} ${PC_FILES} PARENT_SCOPE)
endfunction()

function(create_headers)
    if(ARGN)
        foreach(arg ${ARGN})
            configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${arg}.cmake ${CMAKE_CURRENT_BINARY_DIR}/${arg})
        endforeach()
    else()
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}-config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.h)
    endif()
    include_directories(${CMAKE_CURRENT_BINARY_DIR})
endfunction()

