#!/bin/sh

# start gpe-bluetooth if hardware is built in
if [ -f /etc/sysconfig/bluetooth ]; then
  . /etc/sysconfig/bluetooth
  if [ "$BLUETOOTH" = "yes" ]; then
    exec gpe-bluetooth
  fi
fi
