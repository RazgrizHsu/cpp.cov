
#----------------------------------------------------------------------
#----------------------------------------------------------------------
macro( set_option_category name )
	set( CXXBASE_OPTION_CATEGORY ${name} )
	list( APPEND "CXXBASE_OPTION_CATEGORIES" ${name} )
endmacro()

#----------------------------------------------------------------------
#----------------------------------------------------------------------
macro( DefineBy name description default )
	option( ${name} ${description} ${default} )
	list( APPEND "CXXBASE_${CXXBASE_OPTION_CATEGORY}_OPTION_NAMES" ${name} )
	set( "${name}_OPTION_DESCRIPTION" ${description} )
	set( "${name}_OPTION_DEFAULT" ${default} )
	set( "${name}_OPTION_TYPE" "bool" )
endmacro()

#----------------------------------------------------------------------
#----------------------------------------------------------------------
function( list_join lst glue out )
	if ( "${${lst}}" STREQUAL "" )
		set( ${out} "" PARENT_SCOPE )
		return()
	endif ()

	list( GET ${lst} 0 joined )
	list( REMOVE_AT ${lst} 0 )
	foreach ( item ${${lst}} )
		set( joined "${joined}${glue}${item}" )
	endforeach ()
	set( ${out} ${joined} PARENT_SCOPE )
endfunction()

#----------------------------------------------------------------------
#----------------------------------------------------------------------
macro( DefineBy_string name description default )
	set( ${name} ${default} CACHE STRING ${description} )
	list( APPEND "CXXBASE_${CXXBASE_OPTION_CATEGORY}_OPTION_NAMES" ${name} )
	set( "${name}_OPTION_DESCRIPTION" ${description} )
	set( "${name}_OPTION_DEFAULT" "\"${default}\"" )
	set( "${name}_OPTION_TYPE" "string" )

	set( "${name}_OPTION_ENUM" ${ARGN} )
	list_join( "${name}_OPTION_ENUM" "|" "${name}_OPTION_ENUM" )
	if ( NOT ( "${${name}_OPTION_ENUM}" STREQUAL "" ))
		set_property( CACHE ${name} PROPERTY STRINGS ${ARGN} )
	endif ()
endmacro()

#----------------------------------------------------------------------
#----------------------------------------------------------------------
MACRO( SetCurrentTimeTo CURRENT_TIME )

	execute_process( COMMAND "date" +"%Y-%m-%d %H:%M.%S" OUTPUT_VARIABLE TMPTIME )
	string( REGEX REPLACE "\n" "" TMPTIME ${TMPTIME} )
	string( REGEX REPLACE "\"" "" TMPTIME ${TMPTIME} )
	set( ${CURRENT_TIME} ${TMPTIME} )

ENDMACRO( SetCurrentTimeTo )
