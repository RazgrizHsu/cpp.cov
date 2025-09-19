#==========================================================================================
# Identify OS
#==========================================================================================
if (UNIX)
    if (APPLE)
        set(CMAKE_OS_NAME "mac" CACHE STRING "Operating system name" FORCE)
    else()
        ## Check for Debian GNU/Linux ________________
        find_file(DEBIAN_FOUND debian_version debconf.conf
                PATHS /etc
                )
        if (DEBIAN_FOUND)
            set(CMAKE_OS_NAME "debian" CACHE STRING "Operating system name" FORCE)
        endif (DEBIAN_FOUND)
        ##  Check for Fedora _________________________
        find_file(FEDORA_FOUND fedora-release
                PATHS /etc
                )
        if (FEDORA_FOUND)
            set(CMAKE_OS_NAME "fedora" CACHE STRING "Operating system name" FORCE)
        endif (FEDORA_FOUND)
        ##  Check for RedHat _________________________
        find_file(REDHAT_FOUND redhat-release inittab.RH
                PATHS /etc
                )
        if (REDHAT_FOUND)
            set(CMAKE_OS_NAME "redhat" CACHE STRING "Operating system name" FORCE)
        endif (REDHAT_FOUND)
        ## Extra check for Ubuntu ____________________
        if (DEBIAN_FOUND)
            ## At its core Ubuntu is a Debian system, with
            ## a slightly altered configuration; hence from
            ## a first superficial inspection a system will
            ## be considered as Debian, which signifies an
            ## extra check is required.
            find_file(UBUNTU_EXTRA legal issue
                    PATHS /etc
                    )
            if (UBUNTU_EXTRA)
                ## Scan contents of file
                file(STRINGS ${UBUNTU_EXTRA} UBUNTU_FOUND
                        REGEX Ubuntu
                        )
                ## Check result of string search
                if (UBUNTU_FOUND)
                    set(CMAKE_OS_NAME "ubuntu" CACHE STRING "Operating system name" FORCE)
                    set(DEBIAN_FOUND FALSE)

                    find_program(LSB_RELEASE_EXEC lsb_release)
                    execute_process(COMMAND ${LSB_RELEASE_EXEC} -rs
                            OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT
                            OUTPUT_STRIP_TRAILING_WHITESPACE
                            )
                    STRING(REGEX REPLACE "\\." "_" UBUNTU_VERSION "${LSB_RELEASE_ID_SHORT}")
                endif (UBUNTU_FOUND)
            endif (UBUNTU_EXTRA)
        endif (DEBIAN_FOUND)
    endif (APPLE)
endif (UNIX)


message( "[builder] os[ ${CMAKE_OS_NAME} ]" )

#==========================================================================================
#==========================================================================================
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" )

set( CMAKE_BINARY_DIR ${PROJECT_BINARY_DIR} )

set( DIR_ROOT ${PROJECT_SOURCE_DIR} )
set( Dir_Libs ${PROJECT_SOURCE_DIR}/libs )


if ( NOT DEFINED CMAKE_BUILD_TYPE )
	set( CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." )
endif ()

if ( CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(amd64)|(AMD64)" )
	message( "[root] Building conv on x86 architecture" )
	set( CXXBASE_BUILD_ARCH x86_64 )
elseif ( CMAKE_SYSTEM_PROCESSOR MATCHES "(arm64)" )
	message( "[root] Building conv on ARM64 architecture" )
	set( CXXBASE_BUILD_ARCH arm64 )
else ()
	message( WARNING "Unknown processor type" )
	message( WARNING "CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}" )
	set( CXXBASE_BUILD_ARCH unknown )
endif ()


if ( "${MAKE}" STREQUAL "" )
	find_program( MAKE make )
endif ()

if ( "${CMAKE_MAKE_PROGRAM}" STREQUAL "" )
	find_program( CMAKE_MAKE_PROGRAM Ninja )
endif ()

message( "[PKGs] MAKE[${MAKE}] CMAKE_MAKE_PROGRAM[${CMAKE_MAKE_PROGRAM}]" )



message( "[root] CMAKE_MODULE_PATH[ ${CMAKE_MODULE_PATH} ]" )
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${DIR_ROOT}/cmake" )
