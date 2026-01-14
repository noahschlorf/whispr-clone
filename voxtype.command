#!/bin/bash
cd "$(dirname "$0")"
./build/voxtype -q accurate &
sleep 2
osascript -e 'tell application "Terminal" to close front window' &
exit
