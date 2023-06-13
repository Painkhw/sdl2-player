cmake_minimum_required(VERSION 3.22)
project(ffmpeg-player)
set(ARCH "x86")
set(TOOL_CHAIN "gcc-11.2")


#link_libraries(rt pthread)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fpermissive -std=c17 -pthread")


file(GLOB_RECURSE SOURCEFILES
   # ${PROJECT_SOURCE_DIR}/main.cpp
   ${PROJECT_SOURCE_DIR}/main_thread.c

)

include_directories(${PROJECT_NAME} PUBLIC 
    ${PROJECT_SOURCE_DIR}/3rdparty/ffmpeg-4.4.1/include
    ${PROJECT_SOURCE_DIR}/3rdparty/sdl2/include
    ${PROJECT_SOURCE_DIR}/3rdparty/libx264/include

)

link_directories(${PROJECT_NAME} PUBLIC 
    ${PROJECT_SOURCE_DIR}/3rdparty/ffmpeg-4.4.1/lib
    ${PROJECT_SOURCE_DIR}/3rdparty/sdl2/lib
    ${PROJECT_SOURCE_DIR}/3rdparty/libx264/lib
)

add_executable(${PROJECT_NAME} ${SOURCEFILES} ${HEADERFILES})
target_link_libraries(${PROJECT_NAME}

    avutil
    avformat
    avcodec
    avdevice
    swscale
    swresample
    SDL2
    x264
)

set(OUTPUT "${PROJECT_BINARY_DIR}/ffmpeg-play/")
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${OUTPUT}/)

install(FILES   ${PROJECT_SOURCE_DIR}/run_ffmpeg_play.sh 
                                            DESTINATION ${OUTPUT}/)

