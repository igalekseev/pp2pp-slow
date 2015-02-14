#!/bin/bash

cd /home/daq/pp2pp-slow
check=`ps -e | grep pp2pp-slow-ser | wc -l`

if (($check < 1)) ; then
    mv pp2pp-slow.log pp2pp-slow.log.`date +%F_%H-%M`
    ./pp2pp-slow-server &
fi
