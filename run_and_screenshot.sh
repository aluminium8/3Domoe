#!/bin/bash
./build/GUImain &
PID=$!
sleep 5
xwd -root -out gui_screenshot.xwd
kill $PID
