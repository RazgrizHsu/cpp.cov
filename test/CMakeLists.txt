file(GLOB DIRS_NOW LIST_DIRECTORIES true *)
get_all_paths( DIRS_NOW )

foreach (dir ${DIRS_NOW})
    if( IS_DIRECTORY ${dir} )
        get_filename_component(dir_name ${dir} NAME)

        #file(GLOB_RECURSE dir_files "${dir_name}/**/*.hpp" "${dir_name}/**/*.cpp" )

        file(GLOB_RECURSE dir_files
                "${CMAKE_CURRENT_SOURCE_DIR}/${dir_name}/*.hpp"
                "${CMAKE_CURRENT_SOURCE_DIR}/${dir_name}/*.cpp"
        )

        #message( "test [${dir_name}] files: ${dir_files}" )

        if( dir_files )


            add_executable( test_${dir_name}
                    idx.cpp
                ${dir_files}
            )

            target_link_libraries( test_${dir_name} PRIVATE ${Link_Test} )

            #target_compile_definitions( test_${dir_name} PUBLIC IS_TESTS )

        endif()

    endif()
endforeach ()
