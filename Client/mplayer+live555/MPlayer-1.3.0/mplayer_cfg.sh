#!/bin/sh

./configure --prefix=/home/fishman/GP/mplayer/out --cc=arm-none-linux-gnueabi-gcc --disable-gui  --target=arm-linux --host-cc=gcc  --disable-freetype  --enable-fbdev  --disable-mencoder   --disable-sdl --disable-dvdread --disable-x11 --enable-cross-compile  --disable-dvdnav --disable-jpeg --disable-tga --disable-pnm --disable-tv --disable-ivtv --disable-fontconfig --disable-xanim --disable-win32dll --disable-armv5te --disable-armv6 --enable-static --enable-live  --extra-cflags="-I/home/fishman/GP/mplayer+live555/src/libmad/include -I/home/fishman/GP/mplayer+live555/src/live/liveMedia/include -I/home/fishman/GP/mplayer+live555/src/live/groupsock/include -I/home/fishman/GP/mplayer+live555/src/live/UsageEnvironment/include -I/home/fishman/GP/mplayer+live555/src/live/BasicUsageEnvironment/include" --extra-ldflags="-L/home/fishman/GP/mplayer+live555/src/libmad/lib -L/home/fishman/GP/mplayer+live555/src/live/liveMedia -L/home/fishman/GP/mplayer+live555/src/live/groupsock -L/home/fishman/GP/mplayer+live555/src/live/UsageEnvironment -L/home/fishman/GP/mplayer+live555/src/live/BasicUsageEnvironment"
