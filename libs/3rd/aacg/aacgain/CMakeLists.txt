add_library( aacgain
		aacgain.cpp decoder.c syntax.c MP4MetaFile.cpp )


#message( FATAL_ERROR "aacpath : ${PROJECT_SOURCE_DIR}/mp3gain" )
#message( FATAL_ERROR "aacpath : ${PROJECT_BINARY_DIR}/3rd/include" )

target_include_directories( aacgain PUBLIC
		# Is there a better way to do this than PROJECT_SOURCE_DIR
		"${CMAKE_CURRENT_SOURCE_DIR}"
		"${PROJECT_SOURCE_DIR}/mp3gain"
		"${PROJECT_SOURCE_DIR}/aacgain"

		"${PROJECT_BINARY_DIR}/3rd/include"

		"${PROJECT_SOURCE_DIR}/3rd/mp4v2"
		"${PROJECT_SOURCE_DIR}/3rd/faad2/libfaad"
		# These two exist on UNIX but paths are needed on Windows
		# as we have no install targets for Windows
		"${PROJECT_SOURCE_DIR}/3rd/mp4v2/src"
		"${PROJECT_SOURCE_DIR}/3rd/mp4v2/include"
		"${PROJECT_SOURCE_DIR}/3rd/faad2/include"

		"${PROJECT_BINARY_DIR}/aacgain"
)

target_link_libraries( aacgain PUBLIC
		mp3gain
		mpglib
		faad2
		mp4v2
)

IF ( UNIX )
	target_compile_options( aacgain PRIVATE -Wall -Wextra )
ENDIF ( UNIX )

#install(TARGETS aacgain DESTINATION bin)
