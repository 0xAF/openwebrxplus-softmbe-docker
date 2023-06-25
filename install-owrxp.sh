#!/bin/bash

# TODO: use apt install soapysdr-module-sdrplay3 and test (improved sdrplay support)
# see here: https://luarvique.github.io/ppa/

export MAKEFLAGS="-j12"

function info() {
  if [ -f $1 ]; then
    return 1
  fi
  echo -ne '\n\n\n\n\n==================================================\n'
  echo $2
  echo -ne '==================================================\n\n\n\n\n'
  echo "enter [s] to skip this step, or press enter to continue"
  read answer
  if [ "$answer" == "s" ]; then
    return 1
  fi
}

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

if info /build_init "+ Init..."; then
apt update
apt install -y --no-install-recommends wget gpg ca-certificates auto-apt-proxy udev
touch /build_init
fi

if info /build_s6services "+ Install S6 services..."; then
wget https://github.com/just-containers/s6-overlay/releases/download/v1.21.8.0/s6-overlay-${PLATFORM}.tar.gz
tar xzf s6-overlay-${PLATFORM}.tar.gz -C /
rm s6-overlay-${PLATFORM}.tar.gz
cp -a /files/services/* /etc/services.d/
touch /build_s6services
fi

if info /build_addrepos "+ Add repos and update..."; then
wget -O - https://luarvique.github.io/ppa/openwebrx-plus.gpg | gpg --dearmor -o /etc/apt/trusted.gpg.d/openwebrx-plus.gpg
echo "deb [signed-by=/etc/apt/trusted.gpg.d/openwebrx-plus.gpg] https://luarvique.github.io/ppa/debian ./" > /etc/apt/sources.list.d/openwebrx-plus.list
wget -O - https://repo.openwebrx.de/debian/key.gpg.txt | gpg --dearmor -o /usr/share/keyrings/openwebrx.gpg
echo "deb [signed-by=/usr/share/keyrings/openwebrx.gpg] https://repo.openwebrx.de/debian/ bullseye main" > /etc/apt/sources.list.d/openwebrx.list
apt update
touch /build_addrepos
fi

if info /build_openwebrx "+ Install OpenWebRX, Soapy modules and some libs..."; then
DEBIAN_FRONTEND=noninteractive apt install -y openwebrx soapysdr-module-all libpulse0 libfaad2 libopus0 soapysdr-module-sdrplay3 airspyhf alsa-utils libpopt0 libiio0 libad9361-0 libhidapi-hidraw0 libhidapi-libusb0
touch /build_openwebrx
fi

if info /build_devpackages "+ Install dev packages..."; then
BUILD_PACKAGES="git cmake make patch wget sudo gcc g++ libusb-1.0-0-dev libsoapysdr-dev debhelper cmake libprotobuf-dev protobuf-compiler libcodecserver-dev build-essential xxd qt5-qmake libpulse-dev libfaad-dev libopus-dev libfftw3-dev  pkg-config libglib2.0-dev libconfig++-dev libliquid-dev libairspyhf-dev libpopt-dev libiio-dev libad9361-dev libhidapi-dev libasound2-dev qtmultimedia5-dev  libqt5serialport5-dev qttools5-dev qttools5-dev-tools libboost-all-dev libfftw3-dev libreadline-dev libusb-1.0-0-dev libudev-dev asciidoctor gfortran"
apt-get -y install --no-install-recommends $BUILD_PACKAGES
touch /build_devpackages
fi

if info /build_wsjtx24 "+ Install wsjtx 2.4..."; then
wget https://downloads.sourceforge.net/project/wsjt/wsjtx-2.4.0/wsjtx-2.4.0.tgz
tar fxv wsjtx-2.4.0.tgz
cd wsjtx-2.4.0
mkdir build
cd build
cmake -DWSJT_SKIP_MANPAGES=ON ..
make
make install
cd ../../
rm -rf wsjtx-2.4.0
touch /build_wsjtx24
fi

if info /build_fcdpp "+ Install FCDPP..."; then
git clone https://github.com/pothosware/SoapyFCDPP.git
cd SoapyFCDPP
mkdir build; cd build
cmake ../
make && sudo make install
cd ../../
rm -rf SoapyFCDPP
touch /build_fcdpp
fi

if info /build_plutosdr "+ Install PlutoSDR..."; then
git clone https://github.com/pothosware/SoapyPlutoSDR
cd SoapyPlutoSDR
mkdir build
cd build
cmake ..
make
make install
cd ../../
rm -rf SoapyPlutoSDR
touch /build_plutosdr
fi

if info /build_airspyhf "+ Install AirSpyHF+..."; then
git clone https://github.com/pothosware/SoapyAirspyHF.git
cd SoapyAirspyHF
mkdir build
cd build
cmake ..
make
make install
ldconfig
cd ../../
rm -rf SoapyAirspyHF
touch /build_airspyhf
fi

if info /build_rockprog "+ Install RockProg..."; then
cd /files/rockprog
make
cp rockprog /usr/local/bin/
touch /build_rockprog
fi

if info /build_sdrplay "+ Install SDRPlay..."; then
wget --no-http-keep-alive https://www.sdrplay.com/software/$SDRPLAY_BINARY
sh $SDRPLAY_BINARY --noexec --target sdrplay
patch --verbose -Np0 < /files/sdrplay/install-lib.`uname -m`.patch
cd sdrplay
mkdir -p /etc/udev/rules.d
./install_lib.sh
cd ..
rm -rf sdrplay
rm $SDRPLAY_BINARY
touch /build_sdrplay
fi

if info /build_mbelib "+ Install MBELIB..."; then
git clone https://github.com/szechyjs/mbelib.git
cd mbelib
dpkg-buildpackage
cd ..
dpkg -i libmbe1_1.3.0_*.deb libmbe-dev_1.3.0_*.deb
rm -rf mbelib libmbe_1.3.0*
rm -f *.deb
touch /build_mbelib
fi

if info /build_softmbe "+ Install codecserver-softmbe..."; then
git clone https://github.com/knatterfunker/codecserver-softmbe.git
cd codecserver-softmbe
# ignore missing library linking error in dpkg-buildpackage command
sed -i 's/dh \$@/dh \$@ --dpkg-shlibdeps-params=--ignore-missing-info/' debian/rules
dpkg-buildpackage
cd ..
dpkg -i codecserver-driver-softmbe_0.0.1_*.deb
rm -rf codecserver-softmbe codecserver-driver-softmbe*
rm -f *.deb

# add the softmbe library to the codecserver config
cat >> /etc/codecserver/codecserver.conf << _EOF_
[device:softmbe]
driver=softmbe
_EOF_

touch /build_softmbe
fi


if info /build_perseus "+ Install PerseusSDR..."; then
wget https://github.com/Microtelecom/libperseus-sdr/releases/download/v0.8.2/libperseus_sdr-0.8.2.tar.gz
tar -zxvf libperseus_sdr-0.8.2.tar.gz
cd libperseus_sdr-0.8.2/
./configure
make
make install
ldconfig
cd ..
rm -rf  libperseus*
touch /build_perseus
fi

if info /build_dream "+ Install Dream (DRM)..."; then
wget https://downloads.sourceforge.net/project/drm/dream/2.1.1/dream-2.1.1-svn808.tar.gz
tar xvfz dream-2.1.1-svn808.tar.gz
cd dream
qmake -qt=qt5 CONFIG+=console
make
make install
cd ..
rm -rf dream*
touch /build_dream
fi

if info /build_freedv "+ Install FreeDV..."; then
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
touch /build_freedv
fi

if info /build_hfdl "+ Install HFDL..."; then
git clone https://github.com/szpajder/libacars.git
cd libacars
git checkout v2.1.4
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
touch /build_hfdl
fi

if info /build_patch "+ Patch the web for CPU Temperature..."; then
cd /usr/lib/python3/dist-packages/owrx
patch < /files/temp/cpu.patch
cd /usr/lib/python3/dist-packages/htdocs/lib
patch < /files/temp/progressbar.patch
touch /build_patch
fi

if info /build_clean "+ Clean..."; then
SUDO_FORCE_REMOVE=yes apt-get -y purge --autoremove $BUILD_PACKAGES systemd udev dbus
apt-get clean
rm -rf /var/lib/apt/lists/*
touch /build_clean
fi
