#!/bin/sh

BATTERY_DIRECTORY=`ls /sys/class/power_supply | grep -i "^bat" | head -n 1`

echo "BATTERY_DIRECTORY='\"/sys/class/power_supply/$BATTERY_DIRECTORY\"'"
