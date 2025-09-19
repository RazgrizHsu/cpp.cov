cmake_minimum_required(VERSION 3.10)
project(rz CXX)


#載入子目錄
add_subdirectory( 3rd )


set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

file(GLOB_RECURSE RZ_SOURCES "src/*.cpp" "src/*.c"  )

list(LENGTH RZ_SOURCES RZ_SOURCES_COUNT)

message( "[rz] RZ_SOURCES[${RZ_SOURCES_COUNT}] Files: ${RZ_SOURCES}" )

add_library(rz ${RZ_SOURCES})

set_target_properties( rz PROPERTIES
		CXX_STANDARD 20
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
)

# 設定包含目錄
target_include_directories(rz
		PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

if( APPLE )

target_link_libraries( rz PUBLIC
	iconv
	aacgain
	${Link_rz}
)

else()

	find_library(ICONV_LIB NAMES iconv)
	target_link_libraries(${PROJECT_NAME} )

	target_link_libraries( rz PUBLIC
		aacgain
		${Link_rz}
	)

endif()

#if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
#	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11)
#		message(FATAL_ERROR "GCC version must be at least 11 for C++20 modules support")
#	endif()
#	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmodules-ts")
#elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16)
#		message(FATAL_ERROR "Clang version must be at least 16 for C++20 modules support")
#	endif()
#	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmodules")
#elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
#	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.29)
#		message(FATAL_ERROR "MSVC version must be at least 19.29 for C++20 modules support")
#	endif()
#	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /experimental:module")
#endif()

# 編譯器特定設定（例如 GCC/Clang）
#if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
#	target_compile_options( rz PRIVATE
#			-fmodules
#			-fbuiltin-module-map
#			#-fprebuilt-module-path=${CMAKE_BINARY_DIR}/modules
#	)
#endif()

# 安裝規則
#install(TARGETS rz
#		FILE_SET CXX_MODULES DESTINATION lib/cxx_modules
#)

#export(TARGETS rz FILE rzConfig.cmake)

## target_link_libraries(rz PRIVATE some_other_library)

#install(TARGETS rz
#		EXPORT rzTargets
#		LIBRARY DESTINATION lib
#		ARCHIVE DESTINATION lib
#		RUNTIME DESTINATION bin
#		INCLUDES DESTINATION include
#install(DIRECTORY include/rz DESTINATION include)
#install(EXPORT rzTargets
#		FILE rzTargets.cmake
#		NAMESPACE rz::
#		DESTINATION lib/cmake/rz
#)