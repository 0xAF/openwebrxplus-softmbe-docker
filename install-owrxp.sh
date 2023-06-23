#!/bin/bash

apt update
apt install -y --no-install-recommends wget gpg ca-certificates auto-apt-proxy udev

case `uname -m` in
    arm*)
        PLATFORM=armhf
        SDRPLAY_BINARY=SDRplay_RSP_API-ARM32-3.07.2.run
        ;;
    aarch64*)
        PLATFORM=aarch64
        SDRPLAY_BINARY=SDRplay_RSP_API-ARM64-3.07.1.run
        ;;
    x86_64*)
        PLATFORM=amd64
        SDRPLAY_BINARY=SDRplay_RSP_API-Linux-3.07.1.run
        ;;
esac
echo "+ Install S6 services..."
wget https://github.com/just-containers/s6-overlay/releases/download/v1.21.8.0/s6-overlay-${PLATFORM}.tar.gz
tar xzf s6-overlay-${PLATFORM}.tar.gz -C /
rm s6-overlay-${PLATFORM}.tar.gz

echo "+ Add repos..."
wget -O - https://luarvique.github.io/ppa/openwebrx-plus.gpg | gpg --dearmor -o /etc/apt/trusted.gpg.d/openwebrx-plus.gpg
echo "deb [signed-by=/etc/apt/trusted.gpg.d/openwebrx-plus.gpg] https://luarvique.github.io/ppa/debian ./" > /etc/apt/sources.list.d/openwebrx-plus.list
wget -O - https://repo.openwebrx.de/debian/key.gpg.txt | gpg --dearmor -o /usr/share/keyrings/openwebrx.gpg
echo "deb [signed-by=/usr/share/keyrings/openwebrx.gpg] https://repo.openwebrx.de/debian/ bullseye main" > /etc/apt/sources.list.d/openwebrx.list
apt update
echo "+ Install OpenWebRX, Soapy modules and some libs..."
DEBIAN_FRONTEND=noninteractive apt install -y openwebrx soapysdr-module-all libpulse0 libfaad2 libopus0

echo "+ Install services..."
cp -a /files/services/* /etc/services.d/

echo "+ Install dev packages..."
BUILD_PACKAGES="git cmake make patch wget sudo gcc g++ libusb-1.0-0-dev libsoapysdr-dev debhelper cmake libprotobuf-dev protobuf-compiler libcodecserver-dev build-essential xxd qt5-qmake libpulse-dev libfaad-dev libopus-dev libfftw3-dev  pkg-config libglib2.0-dev libconfig++-dev libliquid-dev"
apt-get -y install --no-install-recommends $BUILD_PACKAGES

echo "+ Install SDRPlay..."
wget --no-http-keep-alive https://www.sdrplay.com/software/$SDRPLAY_BINARY
sh $SDRPLAY_BINARY --noexec --target sdrplay
patch --verbose -Np0 < /files/sdrplay/install-lib.`uname -m`.patch
cd sdrplay
mkdir -p /etc/udev/rules.d
./install_lib.sh
cd ..
rm -rf sdrplay
rm $SDRPLAY_BINARY

export MAKEFLAGS="-j4"
function cmakebuild() {
  cd $1
  if [[ ! -z "${2:-}" ]]; then
    git checkout $2
  fi
  mkdir build
  cd build
  cmake ..
  make
  make install
  cd ../..
  rm -rf $1
}

echo "+ Install Soapy module for SDRPlay..."
git clone https://github.com/pothosware/SoapySDRPlay3.git
# latest from master as of 2021-06-19 (reliability fixes)
cmakebuild SoapySDRPlay3 a869f25364a1f0d5b16169ff908aa21a2ace475d

echo "+ Install MBELIB..."
git clone https://github.com/szechyjs/mbelib.git
cd mbelib
dpkg-buildpackage
cd ..
dpkg -i libmbe1_1.3.0_*.deb libmbe-dev_1.3.0_*.deb
rm -rf mbelib libmbe_1.3.0*
rm -f *.deb

echo "+ Install codecserver-softmbe..."
git clone https://github.com/knatterfunker/codecserver-softmbe.git
cd codecserver-softmbe
# ignore missing library linking error in dpkg-buildpackage command
sed -i 's/dh \$@/dh \$@ --dpkg-shlibdeps-params=--ignore-missing-info/' debian/rules
dpkg-buildpackage
cd ..
dpkg -i codecserver-driver-softmbe_0.0.1_*.deb
rm -rf codecserver-softmbe codecserver-driver-softmbe*
rm -f *.deb

# link the library to the correct location for the codec server
#ln -s /usr/lib/x86_64-linux-gnu/codecserver/libsoftmbe.so /usr/local/lib/codecserver/

# add the softmbe library to the codecserver config
cat >> /etc/codecserver/codecserver.conf << _EOF_
[device:softmbe]
driver=softmbe
_EOF_


echo "+ Install PerseusSDR..."
wget https://github.com/Microtelecom/libperseus-sdr/releases/download/v0.8.2/libperseus_sdr-0.8.2.tar.gz
tar -zxvf libperseus_sdr-0.8.2.tar.gz
cd libperseus_sdr-0.8.2/
./configure
make
make install
ldconfig
cd ..
rm -rf  libperseus*

echo "+ Install Dream (DRM)..."
wget https://downloads.sourceforge.net/project/drm/dream/2.1.1/dream-2.1.1-svn808.tar.gz
tar xvfz dream-2.1.1-svn808.tar.gz
cd dream
qmake -qt=qt5 CONFIG+=console
make
make install
cd ..
rm -rf dream*

echo "+ Install FreeDV..."
git clone https://github.com/drowe67/codec2.git
cd codec2
mkdir build
cd build
cmake ..
make
make install
install -m 0755 src/freedv_rx /usr/local/bin
cd ../..
rm -rf codec2

echo "+ Install HFDL..."
git clone https://github.com/szpajder/libacars.git
git checkout v2.1.4
cd libacars
mkdir build
cd build
cmake ..
make
make install
ldconfig
cd ../../
rm -rf libacars/

git clone https://github.com/szpajder/dumphfdl.git
cd dumphfdl
git checkout v1.4.1
mkdir build
cd build
cmake ..
make
make install
ldconfig
cd ../../
rm -rf dumphfdl

echo "+ Patch the web for CPU Temperature..."
cd /usr/lib/python3/dist-packages/owrx
patch < /files/temp/cpu.patch
cd /usr/lib/python3/dist-packages/htdocs/lib
patch < /files/temp/progressbar.patch

echo "+ Clean..."
SUDO_FORCE_REMOVE=yes apt-get -y purge --autoremove $BUILD_PACKAGES systemd udev dbus
apt-get clean
rm -rf /var/lib/apt/lists/*
