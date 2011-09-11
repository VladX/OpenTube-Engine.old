WHAT IS OPENTUBE?
-----------------

OpenTube is a powerful open source video hosting engine that designed for extremely high loads. It uses built-in HTTP server to handle requests without overheads. OpenTube completely written in C.

BUILDING
-----------------

To build Opentube, you will need CMake build system. If you are building for Linux, you can install the appropriate package from the repositories. CMake for other operating systems can be found on the [official site](http://www.cmake.org/cmake/resources/software.html).

First, you need to generate Makefile

    $ cmake .

Secondly, you need to compile source files

    $ make

or

    $ mingw32-make

if you are using MinGW, or

    $ nmake

if you are using MS Visual C++.

RUNNING
-----------------

To run OpenTube, enter

    $ ./objs/opentube-server

If it does not started, check the log file for errors. To find the location of the log file, view configuration file (server.conf). If the log file is empty or does not exist, check syslog.

REPORTING BUGS
-----------------

Send your bug to <me@xpast.me> or <vvladxx@gmail.com> with the subject "OpenTube bug". You can also use [this bug tracker](https://github.com/VladX/OpenTube-Engine/issues). Thank you.

LICENSE
-----------------

OpenTube is free software and distributed under the terms of the GNU General Public License v3. See COPYING file for details.