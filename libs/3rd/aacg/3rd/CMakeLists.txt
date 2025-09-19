cmake_minimum_required(VERSION 3.14)  # FetchContent requires CMake 3.14+

include(FetchContent)

include(ProcessorCount)
ProcessorCount(N)

# Define the build directory relative to the root project
if(NOT DEFINED DIR_BUILD_3RD)
	set(DIR_BUILD_3RD "${CMAKE_BINARY_DIR}/aac_build")
endif()


message( "------------------------------------------------------------------------" )
message( "aacg 3rds: CBD[${CMAKE_CURRENT_BINARY_DIR}] ABD[${DIR_BUILD_3RD}]" )
message( "------------------------------------------------------------------------" )

#------------------------------------------------------------------------
# faad2 configuration
#------------------------------------------------------------------------
FetchContent_Declare(
		faad2
		SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/faad2"
)

FetchContent_GetProperties(faad2)
if(NOT faad2_POPULATED)
	FetchContent_MakeAvailable(faad2)
endif()

set(FAAD_LIB_PATH "${DIR_BUILD_3RD}/lib/libfaad.a")
add_library(faad2_interface INTERFACE)

add_custom_command(
		OUTPUT "${FAAD_LIB_PATH}"
		COMMAND make clean || true
		COMMAND autoreconf -fi
		COMMAND ./configure --prefix=${DIR_BUILD_3RD} --disable-shared --enable-static
		COMMAND make -j ${N}
		COMMAND make -j ${N} install
		WORKING_DIRECTORY ${faad2_SOURCE_DIR}
)

add_custom_target(build_faad2 DEPENDS "${FAAD_LIB_PATH}")
add_dependencies(faad2_interface build_faad2)

set_target_properties(faad2_interface PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${DIR_BUILD_3RD}/include"
		INTERFACE_LINK_LIBRARIES "${FAAD_LIB_PATH}"
)

add_library(faad2 ALIAS faad2_interface)

#------------------------------------------------------------------------
# mp4v2 configuration
#------------------------------------------------------------------------
FetchContent_Declare(
		mp4v2
		SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mp4v2"
)
FetchContent_GetProperties(mp4v2)
if(NOT mp4v2_POPULATED)
	FetchContent_MakeAvailable(mp4v2)
endif()

set(MP4V2_LIB_PATH "${DIR_BUILD_3RD}/lib/libmp4v2.a")
add_library(mp4v2_interface INTERFACE)

add_custom_command(
		OUTPUT "${MP4V2_LIB_PATH}"
		COMMAND make clean || true
		COMMAND autoreconf -fi
		COMMAND ./configure --prefix=${DIR_BUILD_3RD} --disable-shared --enable-static
		COMMAND make -j ${N}
		COMMAND make -j ${N} install
		WORKING_DIRECTORY ${mp4v2_SOURCE_DIR}
)

add_custom_target(build_mp4v2 DEPENDS "${MP4V2_LIB_PATH}")
add_dependencies(mp4v2_interface build_mp4v2)

set_target_properties(mp4v2_interface PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${DIR_BUILD_3RD}/include"
		INTERFACE_LINK_LIBRARIES "${MP4V2_LIB_PATH}"
)

add_library(mp4v2 ALIAS mp4v2_interface)

#------------------------------------------------------------------------
#
#------------------------------------------------------------------------
set(PREFIX_CLEAN_DIRS ${DIR_BUILD_3RD}/bin ${DIR_BUILD_3RD}/include ${DIR_BUILD_3RD}/lib ${DIR_BUILD_3RD}/share)
set_directory_properties(PROPERTIES ADDITIONAL_CLEAN_FILES "${PREFIX_CLEAN_DIRS}")