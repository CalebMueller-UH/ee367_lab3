#!/usr/bin/bash

clear
make
gnome-terminal -e ./sdir/server 
./cdir/client localhost

