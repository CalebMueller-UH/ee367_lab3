#!/usr/bin/bash

clear
make

current_dir=$(pwd)
gnome-terminal --geometry=100x15 -- $current_dir/sdir/server
gnome-terminal --geometry=100x15 -- $current_dir/cdir/client localhost

echo ""
echo ""
read -p "Press enter to recompile and restart the terminals"
pkill -f "$current_dir/sdir/server"
pkill -f "$current_dir/cdir/client"

./run.sh