#!/bin/sh
echo "starting scan!"
if [ -e /tmp/wifi.scan ] ; then
    /bin/rm -rf /tmp/wifi.scan
fi

for net_list in `iwlist eth0 scan | grep -e "ESSID" | cut -f 2 -d ':' | cut -f 2 -d '"'` ; do
    echo "essid = ${net_list}" >> /tmp/wifi.scan
done
echo "scan complete!"
sleep 2
