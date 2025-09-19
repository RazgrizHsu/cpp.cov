

macro( setTargetForDebug target_name )
	# 確保使用 Debug 模式
	set( CMAKE_BUILD_TYPE Debug )

	# 為目標添加編譯選項
	if ( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
		target_compile_options( ${target_name} PRIVATE
				-g
				-fno-limit-debug-info
				-ffile-compilation-dir=${CMAKE_SOURCE_DIR}
		)

		# 鏈接選項，確保調試信息被包含並壓縮
		set_target_properties( ${target_name} PROPERTIES
				LINK_FLAGS "-Wl,--compress-debug-sections=zlib"
		)
	elseif ( CMAKE_COMPILER_IS_GNUCXX )
		target_compile_options( ${target_name} PRIVATE
				-g
				-fdebug-prefix-map=${CMAKE_SOURCE_DIR}=.
		)
	else ()
		message( WARNING "Unsupported compiler. Debug information embedding might not work as expected." )
	endif ()

	# 輸出一條消息，表示已經為目標設定了調試選項
	message( "[Debug] Configured debugging options for target: ${target_name}" )
endmacro()

#==========================================================================================
# 經由PkgConfig設定某個lib
#==========================================================================================
# 共用的輔助函式，用於處理包含目錄
function(_handle_pkg_include_dirs name libs)
	if(NOT ${name}_INCLUDE_DIRS)
		set(${name}_INCLUDE_DIRS "" PARENT_SCOPE)
		foreach(lib ${libs})
			find_path(${lib}_INCLUDE_DIR ${lib}/${lib}.h)
			if(NOT ${lib}_INCLUDE_DIR)
				find_path(${lib}_INCLUDE_DIR ${lib}.h) #像sqlite3.h 沒有上層目錄
			endif()
			if(${lib}_INCLUDE_DIR)
				list(APPEND include_dirs ${${lib}_INCLUDE_DIR})
				message(STATUS "Found ${lib} include dir: ${${lib}_INCLUDE_DIR}")
			endif()
			unset(${lib}_INCLUDE_DIR CACHE)
		endforeach()
		if(NOT include_dirs)
			message(WARNING "${name}_INCLUDE_DIRS is empty. Assuming system include path.")
		endif()
		set(${name}_INCLUDE_DIRS ${include_dirs} PARENT_SCOPE)
	endif()
endfunction()

# 原始的動態庫設定宏
macro(setByPkgconfig name)
	find_package(PkgConfig REQUIRED)
	set(libs ${ARGN})
	pkg_check_modules(${name} REQUIRED IMPORTED_TARGET ${libs})

	_handle_pkg_include_dirs(${name} "${libs}")

	if(${name}_INCLUDE_DIRS)
		include_directories(${${name}_INCLUDE_DIRS})
	endif()

	# 輸出調試信息
	message(STATUS "${name}_INCLUDE_DIRS: ${${name}_INCLUDE_DIRS}")
	message(STATUS "${name}_LIBRARIES: ${${name}_LIBRARIES}")
	message(STATUS "${name}_LIBRARY_DIRS: ${${name}_LIBRARY_DIRS}")
endmacro()

# 新增的靜態庫設定宏
macro(setByPkgconfigStatic name)
	find_package(PkgConfig REQUIRED)
	set(libs ${ARGN})

	# 正確使用 STATIC 選項，它應該是 pkg_check_modules 的參數，而不是模塊名稱
	pkg_check_modules(${name} REQUIRED IMPORTED_TARGET ${libs})

	_handle_pkg_include_dirs(${name} "${libs}")

	if(${name}_INCLUDE_DIRS)
		include_directories(${${name}_INCLUDE_DIRS})
	endif()

	# 設定靜態庫鏈接標誌
	set(${name}_LINK_FLAGS "-Wl,-Bstatic ${${name}_STATIC_LDFLAGS} -Wl,-Bdynamic")

	# 輸出調試信息
	message(STATUS "${name}_INCLUDE_DIRS: ${${name}_INCLUDE_DIRS}")
	message(STATUS "${name}_STATIC_LIBRARIES: ${${name}_STATIC_LIBRARIES}")
	message(STATUS "${name}_STATIC_LIBRARY_DIRS: ${${name}_STATIC_LIBRARY_DIRS}")
	message(STATUS "${name}_LINK_FLAGS: ${${name}_LINK_FLAGS}")
endmacro()

macro( MacAutoPkgConfigToEnvPaths path )
	# 使用 file(GLOB_RECURSE ...) 來遞迴搜尋所有 .pc 檔案
	file( GLOB_RECURSE _PC_FILES
			"${path}/*.pc"
	)

	if ( _PC_FILES )
		set( _PKGCONFIG_PATHS "" )
		foreach ( _PC_FILE ${_PC_FILES} )
			get_filename_component( _PKGCONFIG_PATH "${_PC_FILE}" DIRECTORY )
			list( APPEND _PKGCONFIG_PATHS "${_PKGCONFIG_PATH}" )
		endforeach ()

		# 移除重複的路徑
		list( REMOVE_DUPLICATES _PKGCONFIG_PATHS )

		foreach ( _PKGCONFIG_PATH ${_PKGCONFIG_PATHS} )
			if ( DEFINED ENV{PKG_CONFIG_PATH} )
				set( ENV{PKG_CONFIG_PATH} "${_PKGCONFIG_PATH}:$ENV{PKG_CONFIG_PATH}" )
			else ()
				set( ENV{PKG_CONFIG_PATH} "${_PKGCONFIG_PATH}" )
			endif ()
			message( "[MacAutoPkgConfig] Added ${_PKGCONFIG_PATH} to PKG_CONFIG_PATH" )
		endforeach ()
	else ()
		message( FATAL_ERROR "No .pc files found in ${path}" )
	endif ()

	# 清理臨時變數
	unset( _PC_FILES )
	unset( _PKGCONFIG_PATHS )
	unset( _PKGCONFIG_PATH )
	unset( _PC_FILE )
endmacro()