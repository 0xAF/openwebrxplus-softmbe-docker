FROM debian:bullseye-slim
MAINTAINER af@0xaf.org
LABEL OpenWebRX+ with Digital codecs (incl. DMR, D-STAR, YSF) and all receivers.


ADD files /files
COPY install-owrxp.sh /
RUN /install-owrxp.sh

COPY run.sh /

ENV S6_CMD_ARG0="/run.sh"
ENTRYPOINT ["/init"]

WORKDIR /

VOLUME /etc/openwebrx
VOLUME /var/lib/openwebrx

CMD []

EXPOSE 8073
