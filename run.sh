#!/bin/bash
set -euo pipefail

PATH=$PATH:/usr/bin:/bin

mkdir -p /etc/openwebrx/openwebrx.conf.d
mkdir -p /var/lib/openwebrx
mkdir -p /tmp/openwebrx/
if [[ ! -f /etc/openwebrx/openwebrx.conf.d/20-temporary-directory.conf ]] ; then
  cat << EOF > /etc/openwebrx/openwebrx.conf.d/20-temporary-directory.conf
[core]
temporary_directory = /tmp/openwebrx
EOF
fi
if [[ ! -f /etc/openwebrx/bands.json ]] ; then
  cp bands.json /etc/openwebrx/
fi
if [[ ! -f /etc/openwebrx/openwebrx.conf ]] ; then
  cp openwebrx.conf /etc/openwebrx/
fi
if [[ ! -z "${OPENWEBRX_ADMIN_USER:-}" ]] && [[ ! -z "${OPENWEBRX_ADMIN_PASSWORD:-}" ]] ; then
  if ! openwebrx admin --silent hasuser "${OPENWEBRX_ADMIN_USER}" ; then
    OWRX_PASSWORD="${OPENWEBRX_ADMIN_PASSWORD}" openwebrx admin --noninteractive adduser "${OPENWEBRX_ADMIN_USER}"
  fi
fi


_term() {
  echo "Caught signal!"
  kill -TERM "$child" 2>/dev/null
}

trap _term SIGTERM SIGINT

openwebrx $@ &

child=$!
wait "$child"

