#!/bin/sh
set -e

make
Xephyr :123 &
export DISPLAY=:123
sleep 0.2
xterm &

./tinywm
