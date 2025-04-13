#!/bin/bash
# Wait for desktop to be ready
/usr/bin/desktop_ready

# Set up vcan0 interface
# SUDO commands must be allowed in the sudoer!
sudo modprobe vcan
sudo ip link add vcan0 type vcan
sudo ip link set vcan0 type can bitrate 500000 restart-ms 100
sudo ip link set vcan0 up

# Start PlotJuggler
/usr/local/bin/plotjuggler &

# Start can_dbc_simulator
/usr/local/bin/can_dbc_simulator /home/kasm-user/plotjuggler-can-dbs/datasamples/test_rav4h.dbc &
