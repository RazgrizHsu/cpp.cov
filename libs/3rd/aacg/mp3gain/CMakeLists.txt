set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(mpglibDBL)

add_library(mp3gain
        STATIC
        mp3gain.c gain_analysis.c apetag.c id3tag.c rg_error.c
        mp3g.cpp
)

target_include_directories(mp3gain PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

if( APPLE )
    target_link_libraries(mp3gain mpglib)
else()
    target_link_libraries(mp3gain mpglib aacgain)
endif()


IF(UNIX)

target_compile_options(mp3gain PRIVATE -Wall -Wextra)

ENDIF(UNIX)
