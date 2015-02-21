#!/bin/bash
# The script should be run from cron.
# It checks that pp2pp-slow-srever is running and responding
# If not:
# - the server is killed
# - logfile is renamed
# - GPIB is reset
# - new server started

snmp_cmd=snmpset
snmp_addr=".1.3.6.1.4.1.318.1.1.4.4.2.1.3"

# APCcmd NUM OUTLET 1/2 (ON/OFF)
APCcmd() 
{
#	echo "APC:" $1 $2 $3
	case $1 in
	0 ) ip="130.199.90.26" ;;
	1 ) ip="130.199.90.38" ;;
	* ) return             ;;
	esac
	$snmp_cmd -v1 -c private $ip ${snmp_addr}.$2 int $3
}

# cycleGPIB
# outlets counts from 1. 1 = ON, 2 = OFF
cycleGPIB()
{
	APCcmd 0 7 2
	APCcmd 1 7 2
	sleep 10
	APCcmd 0 7 1
	APCcmd 1 7 1
}

cd /home/daq/pp2pp-slow
./pp2pp-cmd info >> /dev/null &
sleep 5
IRC=`jobs -r | wc -l`

if (( $IRC > 0 )) ; then
    killall pp2pp-slow-server >> /dev/null
    mv pp2pp-slow.log pp2pp-slow.log.`date +%F_%H-%M`
     cycleGPIB
    ./pp2pp-slow-server >> /dev/null &
fi
