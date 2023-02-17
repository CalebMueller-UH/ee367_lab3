# EE367 Algorithms and Data-Structures Laboratory
## Lab # 3: Client Server
### Description: This lab consists of building a "simple" client and server application that uses sockets, pipes, and forks.

## How to run on the Wiliki filesystem:
This project branch is configured to run with little effort on the behalf of whoever is grading it.  To run this project simply run:
    ./run.sh
in the root file directory (.../lab3/)

This bash script will evoke the project to build itself using the Makefile, and will execute the server and client in the same window.

Please note the following:
- The stderr and stdout output of the server has been silenced in the call within the bashcript.  If for some reason you need to see this output you can execute the client and server through ./cdir/client 127.0.0.1    and    ./sdir/server respectively
- The client has been set to communicate via the local loopback network adapter 127.0.0.1
- To cleanly exit the client be sure to use the 'q' / 'Q' command, as this will exit into the bash script which then cleans up any left over processes
