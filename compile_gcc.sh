#! /bin/sh
gcc main.cpp -g -o ffmpeg_player.out -I /usr/local/include -L /usr/local/lib \
-lSDL2main -lSDL2 -lavformat -lavcodec -lavutil -lswscale

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PWD/../3rdparty/ffmpeg_4.4.1/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PWD/../3rdparty/libx264/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PWD/../3rdparty/sdl/lib

set LD_LIBRARY_PATH $LD_LIBRARY_PATH $PWD/../3rdparty/libx264/lib

echo $LD_LIBRARY_PATH