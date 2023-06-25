# openwebrxplus-softmbe-docker
OpenWebRX+ with codecserver-softmbe (mbelib), enabling DMR, D-Star, YSF, FreeDV, DRM, NXDN and other Digital modes.

I'll try to keep all modes and receivers available in this image.

The image is published on [docker hub](https://hub.docker.com/r/slechev/openwebrxplus-softmbe).

# Prepare
You need to disable the kernel driver for RTL devices (if you're going to use one) and then reboot
```
echo 'blacklist dvb_usb_rtl28xxu' > /etc/modprobe.d/rtl28xx-blacklist.conf      
```

# Install
```
# crete volumes
docker volume create owrxp-settings
docker volume create owrxp-etc

# run container in background
docker run -d --name owrxp-softmbe --device /dev/bus/usb -p 8073:8073 -v owrxp-settings:/var/lib/openwebrx -v owrxp-etc:/etc/openwebrx --restart unless-stopped slechev/openwebrxplus-softmbe

# add admin user
docker exec -it owrxp-softmbe python3 /opt/openwebrx/openwebrx.py admin adduser [username]

```

# More information on the official wiki
* [how to run the container](https://github.com/jketterl/openwebrx/wiki/Getting-Started-using-Docker)
* [adding admin user](https://github.com/jketterl/openwebrx/wiki/User-Management#special-information-for-docker-users)

