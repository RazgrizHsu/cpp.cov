#------------------------------------------------------------------------
# 將所有子資料夾視為一個執行程式
#------------------------------------------------------------------------
file(GLOB DIRS_NOW LIST_DIRECTORIES true *)


# 為特定資料夾定義自定義庫
if( APPLE AND BUILD_GUI )
	set(customFor_ui ${wxWidgets_LIBRARIES})
endif()
# 可為其他資料夾添加類似的定義，例如：
# set(customFor_network "networklib1;networklib2")
# set(customFor_database "sqlitelib;mysqlclient")



foreach (dir ${DIRS_NOW})
	if(IS_DIRECTORY ${dir})
		get_filename_component(dir_name ${dir} NAME)

		# 跳過 ui 目錄，除非明確啟用 GUI 且在 macOS 上
		if(dir_name STREQUAL "ui" AND NOT (APPLE AND BUILD_GUI))
			message(STATUS "[exe:${dir_name}] Skipped (BUILD_GUI is OFF or not on macOS)")
			continue()
		endif()

		file(GLOB_RECURSE SrcFiles "${dir}/*.cpp" "${dir}/*.hpp" "${dir}/*.h" "${dir}/*.c")

		add_executable(${dir_name} ${SrcFiles})

		# 連結通用的庫
		target_link_libraries(${dir_name} ${Link_Srcs})

		# 檢查是否有為該資料夾定義的自定義庫
		if(DEFINED customFor_${dir_name})
			target_link_libraries(${dir_name} ${customFor_${dir_name}})
			message(STATUS "[exe:${dir_name}] Added custom libraries: ${customFor_${dir_name}}")
		else()
			message(STATUS "[exe:${dir_name}] No custom libraries defined")
		endif()

		message(STATUS "[exe:${dir_name}] Finished processing dir [${dir_name}]")
	endif()
endforeach()


if( APPLE AND BUILD_GUI )
	set_target_properties(ui PROPERTIES
			MACOSX_BUNDLE TRUE
			MACOSX_BUNDLE_GUI_IDENTIFIER tw.raz.cov
			MACOSX_BUNDLE_BUNDLE_NAME cov
			MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
			MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
			MACOSX_BUNDLE_COPYRIGHT "© 2024 raz.tw"
	)

	# 設定圖標檔案路徑
	set(APP_PATH_ICON "${CMAKE_SOURCE_DIR}/libs/rss/icon.icns")
	set(APP_PATH_IPNG "${CMAKE_SOURCE_DIR}/libs/rss/ico64.png")


	# 設定 MACOSX_BUNDLE_ICON_FILE 為圖標檔案名（不包含路徑）
	get_filename_component(ICON_FILENAME ${APP_PATH_ICON} NAME)
	set_target_properties(ui PROPERTIES MACOSX_BUNDLE_ICON_FILE ${ICON_FILENAME})

	# 將圖標檔案添加為資源並設定其在包中的位置
	set_source_files_properties(${APP_PATH_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
	set_source_files_properties(${APP_PATH_IPNG} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

	target_sources(ui PRIVATE ${APP_PATH_ICON} ${APP_PATH_IPNG})

# 可選：複製其他資源檔案到 .app 包
# file(GLOB RESOURCE_FILES "${CMAKE_SOURCE_DIR}/resources/*")
# foreach(RESOURCE ${RESOURCE_FILES})
#     set_source_files_properties(${RESOURCE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
#     target_sources(ui PRIVATE ${RESOURCE})
# endforeach()



	# Create a custom Info.plist template
	file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/Info.plist.in
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
<plist version=\"1.0\">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>English</string>
    <key>CFBundleExecutable</key>
    <string>${MACOSX_BUNDLE_EXECUTABLE_NAME}</string>
    <key>CFBundleGetInfoString</key>
    <string>${MACOSX_BUNDLE_INFO_STRING}</string>
    <key>CFBundleIconFile</key>
    <string>${MACOSX_BUNDLE_ICON_FILE}</string>
	<key>CFBundleIconName</key>
	<string>${MACOSX_BUNDLE_ICON_FILE}</string>
    <key>CFBundleIdentifier</key>
    <string>${MACOSX_BUNDLE_GUI_IDENTIFIER}</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleLongVersionString</key>
    <string>${MACOSX_BUNDLE_LONG_VERSION_STRING}</string>
    <key>CFBundleName</key>
    <string>${MACOSX_BUNDLE_BUNDLE_NAME}</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>${MACOSX_BUNDLE_SHORT_VERSION_STRING}</string>
    <key>CFBundleSignature</key>
    <string>????</string>
    <key>CFBundleVersion</key>
    <string>${MACOSX_BUNDLE_BUNDLE_VERSION}</string>
    <key>CSResourcesFileMapped</key>
    <true/>
    <key>NSHumanReadableCopyright</key>
    <string>${MACOSX_BUNDLE_COPYRIGHT}</string>
</dict>
</plist>
			")

	# Configure the Info.plist file
	configure_file(
			${CMAKE_CURRENT_BINARY_DIR}/Info.plist.in
			${CMAKE_CURRENT_BINARY_DIR}/Info.plist
			@ONLY
	)

	set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/Info.plist PROPERTIES MACOSX_PACKAGE_LOCATION "/" )
	target_sources(ui PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)
	set_target_properties(ui PROPERTIES
		MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist
	)
endif()
