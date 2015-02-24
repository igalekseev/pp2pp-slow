#!/bin/bash
# The script resets VME crates

vme_outlet=6
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

# cycle power
# outlets counts from 1. 1 = ON, 2 = OFF
cycleVME()
{
	APCcmd 0 $vme_outlet 2
	APCcmd 1 $vme_outlet 2
	sleep 10
	APCcmd 0 $vme_outlet 1
	APCcmd 1 $vme_outlet 1
}

cycleVME

