#!/bin/bash
set -euxo pipefail
export MAKEFLAGS="-j12"

echo "+ init..."
apt update
apt-get -y install --no-install-recommends wget gpg ca-certificates

echo "+ Add repos and update..."
wget -O - https://luarvique.github.io/ppa/openwebrx-plus.gpg | gpg --dearmor -o /etc/apt/trusted.gpg.d/openwebrx-plus.gpg
echo "deb [signed-by=/etc/apt/trusted.gpg.d/openwebrx-plus.gpg] https://luarvique.github.io/ppa/debian ./" > /etc/apt/sources.list.d/openwebrx-plus.list
wget -O - https://repo.openwebrx.de/debian/key.gpg.txt | gpg --dearmor -o /usr/share/keyrings/openwebrx.gpg
echo "deb [signed-by=/usr/share/keyrings/openwebrx.gpg] https://repo.openwebrx.de/debian/ bullseye main" > /etc/apt/sources.list.d/openwebrx.list
apt update

echo "+ Install dev packages..."
BUILD_PACKAGES="git build-essential debhelper cmake libprotobuf-dev protobuf-compiler libcodecserver-dev wget gpg"
apt-get -y install --no-install-recommends $BUILD_PACKAGES

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

# add the softmbe library to the codecserver config
ln -s /usr/lib/x86_64-linux-gnu/codecserver/libsoftmbe.so /usr/local/lib/codecserver/
mkdir -p /usr/local/etc/codecserver
#cat >> /etc/codecserver/codecserver.conf << _EOF_
cat >> /usr/local/etc/codecserver/codecserver.conf << _EOF_

# add softmbe
[device:softmbe]
driver=softmbe
_EOF_

#mkdir -p /etc/services.d/codecserver
#cat >> /etc/services.d/codecserver/run << _EOF_
##!/usr/bin/execlineb -P
#/usr/bin/codecserver
#_EOF_

sed -i 's/set -euo pipefail/set -euo pipefail\ncd \/opt\/openwebrx/' /opt/openwebrx/docker/scripts/run.sh
sed -i 's/set -euo pipefail/set -euo pipefail\ncd \/opt\/openwebrx/' /run.sh

echo "+ Clean..."
SUDO_FORCE_REMOVE=yes apt-get -y purge --autoremove $BUILD_PACKAGES
apt-get clean
rm -rf /var/lib/apt/lists/*
rm -rf /files
