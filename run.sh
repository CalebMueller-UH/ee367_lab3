#!/usr/bin/bash

clear
make

gnome-terminal -e ./sdir/server 
gnome-terminal -e ./cdir/client localhost 


