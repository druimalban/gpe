#!/bin/sh

if [ -f /etc/sysconfig/bluetooth ]; then
  . /etc/sysconfig/bluetooth

  exec /sbin/hciattach -n $BLUETOOTH_PORT $BLUETOOTH_PROTOCOL $BLUETOOTH_SPEED
else
  echo "Bluetooth not configured"
  exit 1
fi

