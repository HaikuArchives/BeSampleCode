#!/bin/sh

cd `dirname $0`

alert "This will install the InputRecorder.

It will inform you of progress by periodically using alert boxes like this." "Cancel" "Ok"
if [ $? -ne 1 ]
then
	exit 0
fi

# InputRecorder add-ons are loaded twice in installed in /boot/home as of R4
SYSTEMINPUTADDONS="/boot/beos/system/add-ons/input_server"
INPUTADDONS="/boot/home/config/add-ons/input_server"

# If they exist, remove the heavily obsolete device/filter
rm -f $SYSTEMINPUTADDONS/inputRecorderDevice $SYSTEMINPUTADDONS/inputRecorderDevice.xMAP
rm -f $SYSTEMINPUTADDONS/inputRecorderFilter $SYSTEMINPUTADDONS/inputRecorderFilter.xMAP

if [ -e "$SYSTEMINPUTADDONS/devices/InputRecorderDevice" -o -e "$SYSTEMINPUTADDONS/filters/InputRecorderFilter" ]
then
	alert "You have the old InputRecorder device/filter installed under $SYSTEMINPUTADDONS. As of R4.5 that is not needed. The new add-ons will be located in $INPUTADDONS instead. The old ones will be removed." "Cancel" "Ok"
	if [ $? -ne 1 ]
	then
		exit 0
	fi
	rm -f $SYSTEMINPUTADDONS/devices/InputRecorderDevice $SYSTEMINPUTADDONS/devices/InputRecorderDevice.xMAP
	rm -f $SYSTEMINPUTADDONS/filters/InputRecorderFilter $SYSTEMINPUTADDONS/filters/InputRecorderFilter.xMAP
fi

alert "Will now install the input device and filter, as well as copy the InputRecorder to /boot/home/config/bin and provide a symlink on your Desktop." "Cancel" "Ok"
if [ $? -ne 1 ]
then
	exit 0
fi

case `uname -m` in
BePC)
	CPU=x86
	;;
BeMac|BeBox)
	CPU=ppc
	;;
esac

function install_to() {
	if [ -e "$CPU/$1" ]
	then
		mkdir -p "$2"
		cp -av "$CPU/$1" "$2/$1"
		mimeset -f "$2/$1"
		if [ "$#" -ge 3 ]
		then
			mkdir -p "$3"
			ln -svf "$2/$1" "$3/$1"
		fi
	fi
}

rm -f /boot/home/config/bin/InputRecorder /boot/home/config/bin/InputRecorder.xMAP
install_to InputRecorder /boot/home/config/bin /boot/home/Desktop
install_to InputRecorderDevice $INPUTADDONS/devices
install_to InputRecorderFilter $INPUTADDONS/filters

alert "Will now restart input server.

This is needed to start using the InputRecorder right away. If you do not do this, you will not be able to record/play input until you reboot." "Not now" "Ok"
if [ $? -eq 1 ]
then
	/system/servers/input_server -q
	sleep 1
	waitfor _input_server_event_loop_
else
	alert "Input device/filter will load at next reboot."
fi

alert "Install Complete" "Ok"

