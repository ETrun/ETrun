ETrun - The W:ET timerun mod
============================

[![Coverity Status](https://scan.coverity.com/projects/3689/badge.svg?flat=1)](https://scan.coverity.com/projects/3689)

ETrun is a Wolfenstein: Enemy Territory game modification based on ET-GPL.
The objective of this mod is to bring timeruns support to it.

**Visit https://timeruns.net for more information.**

Video
=====

[![Link to the video](https://img.youtube.com/vi/asMrNNIT0e0/0.jpg)](https://www.youtube.com/watch?v=asMrNNIT0e0)

Compatibility
=============

Windows
-------

Game modules require [Microsoft Visual C++ 2013 Redistributable Package](https://www.microsoft.com/en-us/download/details.aspx?id=40784) to run properly.
Works (tested) on `Windows 7, 8, 8.1, 10` with `ET 2.60b`.

Linux
-----

Requires `GLIBC >= 2.7`.
Works (tested) on `Debian 6`, `Linux Mint 12` with `ET 2.60b` and `ET: Legacy`.

MacOS
-----

Not compatible with `ET 2.60d`.
Works (tested) on `MacOS 10.13` with `ET: Legacy`.

Documentation
=============

[ETrun documentation](https://etrun.readthedocs.io/en/latest/).

Building ETrun
==============

Get ETrun source code with its submodule:

	$ git clone --recursive git@github.com:ETrun/ETrun.git

Then run the build script:

	$ ./make.sh -h
	$ ./make.sh [OPTIONS]

Alternatively you can point the CMake GUI tool to the CMakeLists.txt file and generate platform specific build project or IDE workspace.

Docker
------

Docker can be used to build the mod for Linux:

	$ docker build -f Dockerfile.build -t etrun-build .
	$ docker run --rm -v "$PWD":/code -w /code etrun-build ./make.sh [OPTIONS]

Testing
=======

There are test scripts for Windows and Linux / MacOS located in `test` directory.
Before using them you need to setup your config file. Make a copy of the `*.config.example` file corresponding to your OS and run the following command to know all available options:

	$ ./test/unix/test.sh -h

Credits
=======

* TJMod developers
* Racesow project
* ETLegacy project
* iodfe project
