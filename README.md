ETrun
=====

ETrun is a Wolfenstein: Enemy Territory game modification based on ET-GPL.
The objective of this mod is to bring timeruns support to it.
ETrun is currently under development.

**Visit www.timeruns.net for more information.**

Key features
------------

* Many bugfixes and exploit fixes
* Support of +forward turning
* Doublejump
* Timer reset fix
* Velocity jumppads & location jumppads support
* Fixed kill & hurt triggers
* Introduced [ETrun cvars](https://github.com/boutetnico/ETrun/wiki/ETrun-cvars)
* New scoreboard
* No overbounce mode
* Slick control

Credits
-------

* TJMod developpers
* Racesow project

Bootstrapping
-------------

On Linux, make sure you have the following packages installed:

* gcc
* autoconf
* automake
* libtool
* libcurl dev packages

On OSX, make sure to have autoconf, libtool and libidn. You can install them via homebrew.

Then, run the bootstrap script:

<pre>
$ ./bootstrap
</pre>

or run bootstrap.bat if you are running Windows.

Compiling for Windows
---------------------

On Windows: use MSVC compiler.

On Linux, not supported.

Windows server module requires [Microsoft Visual C++ 2012 Redistributable Package](http://www.microsoft.com/fr-fr/download/details.aspx?id=30679) to run properly.

Compiling for Linux (32-bit)
----------------------------

On Windows: not supported.

On Linux, run:

<pre>
$ ./linux_compile.sh
</pre>

Compiling for OSX
-----------------

On OSX, run:

<pre>
$ ./osx_compile.sh
</pre>

You can also use Xcode projects located in src directory.
