#==========================================================================================
# settings
#==========================================================================================

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC") #共享庫

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-but-set-variable")		#略過未使用的變數
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument") #略過未使用的編譯參數



# prefer pthread
set( THREADS_PREFER_PTHREAD_FLAG ON )
find_package( Threads REQUIRED )



#==========================================================================================
# libs
#==========================================================================================
findLib( archive archive.h archive )


##==========================================================================================
#
##==========================================================================================

setByPkgconfig( OpenCV opencv4 )


if( APPLE )
	MacAutoPkgConfigToEnvPaths( "/opt/homebrew/Cellar/icu4c@77" )
else()
endif()

setByPkgconfigStatic( ICU
	icu-uc
	icu-io
	icu-i18n
)


#setByPkgconfig( sqlite sqlite3 )

# macOS: 上使用 Homebrew 安裝的psql
# if(APPLE)
# 	list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/libpq")
#
# 	find_package(PostgreSQL REQUIRED)
# 	include_directories(${PostgreSQL_INCLUDE_DIRS})
#
# endif()



if( APPLE ) #在ubuntu好像包含在libc6-dev裡

findLibStatic( iconv iconv.h iconv )
else()

#	find_package(PkgConfig REQUIRED)
#	pkg_search_module(ICONV REQUIRED iconv)
#	target_link_libraries(${PROJECT_NAME} ${ICONV_LIBRARIES})

endif()



if( APPLE AND BUILD_GUI )
#------------------------------------------------------------------
# ui
#------------------------------------------------------------------
find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

#message( "wx: ${wxWidgets_LIBRARIES}")

endif()

#==========================================================================================
# OATPP
#==========================================================================================
#set(OATPP_BUILD_TESTS OFF)
#set(OATPP_LINK_TEST_LIBRARY OFF)
#
#FetchContent_Declare(
#	oatpp
#	GIT_REPOSITORY https://github.com/oatpp/oatpp.git
#	GIT_TAG ${VER_OATPP}
#)
#FetchContent_MakeAvailable(oatpp)
#
#if(NOT TARGET oatpp)
#	add_library(oatpp INTERFACE IMPORTED)
#	set_target_properties(oatpp PROPERTIES
#		INTERFACE_INCLUDE_DIRECTORIES "${oatpp_SOURCE_DIR}/include"
#	)
#endif()
#
#include_directories("${oatpp_SOURCE_DIR}")
