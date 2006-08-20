#!/bin/sh

if [ -f /etc/sysconfig/bluetooth ]; then
  . /etc/sysconfig/bluetooth

  if [ -f "$BLUETOOTH_SCRIPT" ]; then
    BTS="-S $BLUETOOTH_SCRIPT"
  fi

  exec /sbin/hciattach -n $BLUETOOTH_PORT $BLUETOOTH_PROTOCOL $BLUETOOTH_SPEED $BTS

else
  echo "Bluetooth not configured"
  exit 1
fi

