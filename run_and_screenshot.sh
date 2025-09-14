#!/bin/bash
set -e

# xvfb-run ./build/GUImain &
# PID=$!
# sleep 5
# xwd -root -display :99 -out gui_screenshot.xwd
# kill $PID

xvfb-run -a --server-args="-screen 0 1280x720x24" ./build/GUImain &
PID=$!
sleep 5

# Get the window ID of the application
WINDOW_ID=$(xdotool search --pid $PID | head -1)

if [ -z "$WINDOW_ID" ]; then
    echo "Could not find window ID for PID $PID"
    kill $PID
    exit 1
fi
echo "Found window ID: $WINDOW_ID"


# Capture a screenshot of the specific window
xwd -id $WINDOW_ID -out gui_screenshot.xwd

# Convert the screenshot to a more common format (e.g., PNG)
convert gui_screenshot.xwd gui_screenshot.png

echo "Screenshot saved to gui_screenshot.xwd and gui_screenshot.png"

kill $PID
