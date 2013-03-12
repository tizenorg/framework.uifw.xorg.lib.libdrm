#!/bin/sh

USER=`whoami`
PREFIX=/usr/local/build
KERNEL_DIR=/home/pub/git/party/linux-mobile
CROSS_COMPILER="/opt/toolchains/arm-linux-link/bin/arm-none-linux-gnueabi-"

if [ "$USER" = "dofmind" ]; then
	PREFIX=/usr/local/build
	KERNEL_DIR=/home/pub/git/gerrit/linux-2.6
	CROSS_COMPILER="/opt/toolchains/arm-linux-link/bin/arm-none-linux-gnueabi-"
fi
if [ "$USER" = "daeinki" ]; then
	PREFIX=/home/daeinki/project/share
	KERNEL_DIR=/home/daeinki/project/s5pc210/linux-mobile
	CROSS_COMPILER="/usr/local/arm/arm-2009q3-93/bin/arm-none-linux-gnueabi-"
fi

HOST="`echo $CROSS_COMPILER | sed 's/.*bin\///' | sed 's/-$//'`"

make distclean
./autogen.sh
./configure --host=$HOST --prefix=$PREFIX --disable-intel --disable-radeon --enable-exynos-experimental-api --with-kernel-source=$KERNEL_DIR

sudo rm -rf $PREFIX
sudo mkdir -p $PREFIX
sudo chown $USER.$USER $PREFIX

make
make install

mkdir $PREFIX/bin
cp -a ./tests/gemtest/.libs/gemtest $PREFIX/bin
cp -a ./tests/kmstest/.libs/kmstest $PREFIX/bin
cp -a ./tests/modeprint/.libs/modeprint $PREFIX/bin
cp -a ./tests/modetest/.libs/modetest $PREFIX/bin
cp -a ./tests/proptest/.libs/proptest $PREFIX/bin
cp -a ./tests/ipptest/.libs/ipptest $PREFIX/bin
cp -a ./tests/g2dtest/.libs/g2dtest $PREFIX/bin
cp -a ./tests/rottest/.libs/rottest $PREFIX/bin

tar zcvf libdrm.tar.gz $PREFIX
