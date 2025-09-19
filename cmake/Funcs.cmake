

function(get_all_paths result)
	set(options "")
	set(oneValueArgs DIR)
	set(multiValueArgs "")
	cmake_parse_arguments(PARSE_ARGV 1 ARG "${options}" "${oneValueArgs}" "${multiValueArgs}")

	if(NOT DEFINED ARG_DIR)
		set(ARG_DIR ${CMAKE_CURRENT_SOURCE_DIR})
	endif()

	file(GLOB children RELATIVE ${ARG_DIR} ${ARG_DIR}/*)
	set(itemlist "")
	foreach(child ${children})
		if(IS_DIRECTORY ${ARG_DIR}/${child})
			list(APPEND itemlist ${ARG_DIR}/${child})
			get_all_paths(subdir_list DIR ${ARG_DIR}/${child})
			list(APPEND itemlist ${subdir_list})
		else()
			list(APPEND itemlist ${ARG_DIR}/${child})
		endif()
	endforeach()
	set(${result} ${itemlist} PARENT_SCOPE)
endfunction()


# get_all_subdirs
function(get_all_subdirs result dir)
	file(GLOB children RELATIVE ${dir} ${dir}/*)
	set(dirlist "")
	foreach(child ${children})
		if(IS_DIRECTORY ${dir}/${child})
			list(APPEND dirlist ${dir}/${child})
			get_all_subdirs(subdir_list ${dir}/${child})
			list(APPEND dirlist ${subdir_list})
		endif()
	endforeach()
	set(${result} ${dirlist} PARENT_SCOPE)
endfunction()


# 內部共用函式
function( _findLib_internal id names_include names_bin static_only )
	unset(_dir_incs CACHE)
	unset(_dir_bins CACHE)

	message( "[findLib] 查詢: id[${id}] names_include[${names_include}] names_bin[${names_bin}] static_only[${static_only}]" )

	# 保存原始的庫搜尋後綴
	set(_original_find_library_suffixes ${CMAKE_FIND_LIBRARY_SUFFIXES})

	# 根據 static_only 參數設定庫搜尋選項
	if(static_only)
		set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
		message("[findLib] 僅搜尋靜態庫 (.a)")
	endif()

	if(APPLE)
		execute_process(COMMAND uname -m OUTPUT_VARIABLE ARCHITECTURE OUTPUT_STRIP_TRAILING_WHITESPACE)
		if(ARCHITECTURE STREQUAL "arm64")
			message("[findLib] 在 ARM64 Mac 的 Homebrew 路徑中搜尋")
			find_library(_dir_bins ${names_bin}
					PATHS
					/opt/homebrew/lib
					/opt/homebrew/opt/${names_bin}/lib
					/opt/homebrew/opt/lib${names_bin}/lib
					/usr/local/opt/${names_bin}/lib
					NO_DEFAULT_PATH
			)
		else()
			message("[findLib] 在 Intel Mac 的 Homebrew 路徑中搜尋")
			find_library(_dir_bins ${names_bin}
					PATHS
					/usr/local/lib
					/usr/local/opt/${names_bin}/lib
					NO_DEFAULT_PATH
			)
		endif()

		if(NOT _dir_bins)
			message("[findLib] Homebrew 庫未找到，回退到系統路徑")
			find_library(_dir_bins ${names_bin})
		endif()
	else() # Linux 和其他系統
		# 檢測系統架構
		execute_process(COMMAND uname -m OUTPUT_VARIABLE ARCHITECTURE OUTPUT_STRIP_TRAILING_WHITESPACE)

		# 添加架構特定的庫路徑
		if(ARCHITECTURE STREQUAL "aarch64")
			set(ARCH_LIB_PATHS
					/usr/lib/aarch64-linux-gnu
					/usr/local/lib/aarch64-linux-gnu
			)
		elseif(ARCHITECTURE STREQUAL "x86_64")
			set(ARCH_LIB_PATHS
					/usr/lib/x86_64-linux-gnu
					/usr/local/lib/x86_64-linux-gnu
			)
		elseif(ARCHITECTURE STREQUAL "i686")
			set(ARCH_LIB_PATHS
					/usr/lib/i386-linux-gnu
					/usr/local/lib/i386-linux-gnu
			)
		endif()

		# 搜尋架構特定路徑和標準路徑
		find_library(_dir_bins ${names_bin}
				PATHS
				${ARCH_LIB_PATHS}
				/usr/lib
				/usr/local/lib
				"$ENV{LD_LIBRARY_PATH}"
				"$ENV{CMAKE_PREFIX_PATH}/lib"
		)

		if(NOT _dir_bins)
			message("[findLib] 在預設路徑中未找到庫，回退到系統路徑")
			find_library(_dir_bins ${names_bin})
		endif()
	endif()

	# 恢復原始的庫搜尋後綴
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${_original_find_library_suffixes})

	if(_dir_bins)
		get_filename_component(_base_path "${_dir_bins}" PATH)
		get_filename_component(_base_path "${_base_path}" PATH)

		message(STATUS "[${id}] 找到庫: [${_dir_bins}] 使用基礎路徑[${_base_path}]")

		find_path(_dir_incs ${names_include}
				PATHS
				"${_base_path}/include"
				"${_base_path}/opt/${names_bin}/include"
				"${_base_path}/opt/lib${names_bin}/include"
				"/opt/homebrew/include"
				"/usr/local/include"
				"/usr/include"
				NO_DEFAULT_PATH
		)

		if(NOT _dir_incs)
			message(STATUS "[${id}] 在預設路徑中未找到頭檔案，搜尋系統路徑")
			find_path(_dir_incs ${names_include})
		endif()
	else()
		message(FATAL_ERROR "[${id}] 未找到 [${names_bin}] 的庫檔案")
	endif()

	if(NOT _dir_incs)
		message(FATAL_ERROR "[${id}] 未找到 [${names_include}] 的頭檔案")
	endif()

	if (_dir_incs AND _dir_bins)
		add_library(${id} INTERFACE IMPORTED)
		set_target_properties(${id} PROPERTIES
				INTERFACE_INCLUDE_DIRECTORIES "${_dir_incs}"
				INTERFACE_LINK_LIBRARIES "${_dir_bins}"
		)
		include_directories("${_dir_incs}")

		message("[findLib] 成功[${id}] 頭檔案路徑[${_dir_incs}] 庫檔案路徑[${_dir_bins}]")
	else()
		message(FATAL_ERROR "[${id}] 找到但路徑未解析")
	endif()

	# 將結果傳回給調用函式
	set(_dir_incs "${_dir_incs}" PARENT_SCOPE)
	set(_dir_bins "${_dir_bins}" PARENT_SCOPE)
endfunction()

# 原始 findLib 函式 - 保持不變
function( findLib id names_include names_bin )
	_findLib_internal( ${id} ${names_include} ${names_bin} FALSE )
endfunction()

# 新增的 findLibStatic 函式 - 只搜尋靜態庫
function( findLibStatic id names_include names_bin )
	_findLib_internal( ${id} ${names_include} ${names_bin} TRUE )
endfunction()