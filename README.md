# NetCat-Tool-for-Multi-Connection

## Overview

This Project is based on the implementation of `NetCat` by establishing the TCP connection on C with 2 types of Implementations either `Poll` or `Thread`.
1. the command formatting is listed here. ` nc [-k] [-l] [-v] [-r]  [-p source_port] [-w timeout] [hostname] [port]`

## Configuring your environment & Usages

To start using this project, you need to get your computer configured so you can build and execute the code.
To do this, follow these steps; the specifics of each step (especially the first two) will vary based on which operating system your computer has:

1. [Install git](https://git-scm.com/downloads) (v2.X). After installing you should be able to execute `git --version` on the command line.

1. Clone your repository by running `git clone REPO_URL` from the command line. 

1. Run `MAKEFILE ncP` for Poll options and do `ncP nc [-k] [-l] [-v] [-r]  [-p source_port] [-w timeout] [hostname] [port]`.

1. Run `MAKEFILE ncTh` for Thread options and do `ncTh nc [-k] [-l] [-v] [-r]  [-p source_port] [-w timeout] [hostname] [port]`.

1. Run `MAKEFILE clean` to clean up all make stuff.


#### more examples are listed at the end of the document. Can also use `gcc` to make the files if needed.

## Project commands

Once your environment is configured you need to further prepare the project's tooling and dependencies.
In the project folder:

1. `-k` to force nc to stay listening for another connection after its current connection is completed. It is an error to use this option without the -l option.

1. `-l` to specify that nc should listen for an incoming connection rather than initiating  a connection to a remote host. Any timeouts specified with the -w option are ignored. It is an error to use this option with the -p option.

1. `-p` #source_port# Specifies the source port nc should use, subject to privilege restrictions and availability. It is an error to use this option in conjunction with the -l option. If this option is not given then allow the system to choose the source port.

1. `-v` enables verbose tracing for the purposes of debugging usages only.

1. `-w` #timeout# If a connection and stdin are idle for more than timeout seconds, then the connection is silently closed. The -w flag has no effect when used with the -l option (i.e. in server mode). A server will listen forever for a connection, with or without the -w flag and it will never timeout connections. The default is no timeout.

1. `-r` This option can only be used with the -l option. When this option is specified the server is to work with up to 5 connections simultaneously. In this configuration any data read from any of the network connections is forwarded to standard output, and all the other network connections, except the one it arrived on. Any data read from standard input is resent to all of the network connections, but is not sent to standard output. When printing the output do not add any additional information; print only what is received.

##### NOTE: When -r is used without -k, but -l, then, after the first connection is accepted, when the number of connections goes to 0 the server is to exit. However, if -k and -l are used with -r then the server will listen indefinitely for additional connections.

## Examples 

1. `nc -p 31337 -w 5 host.example.com 42` Open a TCP connection to port 42 of host.example.com, using port 31337 as the source port, with a timeout of 5 seconds

1. `nc -l -r -k 3456` This starts a server listening on port 3456 that copies the input from any network connection to all the other network connections and to standard output or from standard input to all the network connections. The -k option means the server will continue to listen even if there are no clients connected. The server can have up to 5 connections open at once. When a connection is opened, only data that arrives after that connection is opened is sent to this newly opened connection.
