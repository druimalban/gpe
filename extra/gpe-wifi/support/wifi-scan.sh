#!/bin/sh
echo "starting scan!"
if [ -e /tmp/wifi.scan ] ; then
    /bin/rm -rf /tmp/wifi.scan
fi
connected=`iwconfig eth0 | grep -e "ESSID" | cut -f 2 -d ':' | cut -f 2 -d '"'`
echo "connected = ${connected}" >> /tmp/wifi.scan

for net_list in `iwlist eth0 scan | grep -e "ESSID" | cut -f 2 -d ':' | cut -f 2 -d '"'` ; do
	if [ "$connected" != "$net_list" ] ; then 
    		echo "essid = ${net_list}" >> /tmp/wifi.scan
	fi
done
echo "scan complete!"
sleep 2
