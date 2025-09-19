message( "========================================================================" )
message( "=> index start" )
message( "========================================================================" )
#========================================================================
cmake_policy( SET CMP0048 NEW )
cmake_minimum_required( VERSION 3.20 )
#========================================================================
project( conv LANGUAGES CXX )
project( conv VERSION 0.1 )

set( CMAKE_EXPORT_COMPILE_COMMANDS ON ) # 建立compiler_commands.json給clangd用
set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

set( CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "TRUE" )

set( CMAKE_BUILD_TYPE Debug )


if ( CMAKE_BUILD_TYPE STREQUAL "Debug" )
	if ( CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" )
		#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpedantic -Werror") #這是將所有警告設定為異常

		set( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g" )
		set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g" )
	elseif ( MSVC )
		set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W4 /DEBUG" )
	endif ()
endif ()


if ( APPLE )
	set( CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build )
	set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build )
else ()
	set( CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build_linux )
	set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build_linux )
endif ()


#include(CheckCXXCompilerFlag)
#check_cxx_compiler_flag("-fdebug-prefix-map=/old=/new" COMPILER_SUPPORTS_DEBUG_PREFIX_MAP)
#
#if(COMPILER_SUPPORTS_DEBUG_PREFIX_MAP)
#	# 定義源程式碼的絕對路徑和期望的調試路徑
#	set(SOURCE_PATH ${CMAKE_SOURCE_DIR})
#	set(DEBUG_PATH "/src")
#
#	# 添加編譯選項
#	add_compile_options(-fdebug-prefix-map=${SOURCE_PATH}=${DEBUG_PATH})
#else()
#	message(WARNING "Compiler does not support -fdebug-prefix-map")
#endif()


#========================================================================
include( CheckIncludeFile )
include( CheckIncludeFileCXX )
include( CheckIncludeFiles )
include( ExternalProject )
include( FetchContent )

include( cmake/Defines.cmake )
include( cmake/Builder.cmake )
include( cmake/Funcs.cmake )
include( cmake/Macros.cmake )


SetCurrentTimeTo( BUILD_TIME )
#========================================================================
DefineBy( BUILD_TEST "是否要編譯測試" ON )
DefineBy( BUILD_GUI "是否要編譯 GUI 應用程式" OFF )

message( "[root] CMake[${CMAKE_VERSION}] time[${BUILD_TIME}]" )


include( cmake/Packages.cmake )

message( "[root] CMAKE_C_COMPILER[${CMAKE_C_COMPILER}] CMAKE_CXX_COMPILER[${CMAKE_CXX_COMPILER}]" )
if ( CMAKE_CXX_COMPILER_ID )
	message( STATUS "C++ Compiler ID: ${CMAKE_CXX_COMPILER_ID}" )
endif ()

if ( CMAKE_CXX_COMPILER_VERSION )
	message( STATUS "C++ Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}" )
endif ()


#------------------------------------
# links
#------------------------------------
set( Link_rz
		PkgConfig::ICU
)

set( Link_Srcs
		pthread
		rz
		archive

		PkgConfig::ICU
		PkgConfig::OpenCV
)

set( Link_Test
		pthread

		rz
		archive


		aacgain
		PkgConfig::ICU
		PkgConfig::OpenCV
)

#------------------------------------------------------------------------
# dirs
#------------------------------------------------------------------------
add_subdirectory( libs )
add_subdirectory( src )


#------------------------------------------------------------------------
# Unit Test
#------------------------------------------------------------------------
if ( BUILD_TEST )
	add_subdirectory( ${CMAKE_CURRENT_SOURCE_DIR}/test )
endif ()
