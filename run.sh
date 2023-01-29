#!/usr/bin/bash

clear
make

./sdir/server > /dev/null 2>&1 &
sleep 1
./cdir/client 127.0.0.1

pkill -f ./sdir/server
pkill -f ./cdir/client


