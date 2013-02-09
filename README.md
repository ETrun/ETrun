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
* Introduced [ETrun client commands](https://github.com/boutetnico/ETrun/wiki/ETrun-client-commands)
* more... ([ChangeLog](https://github.com/boutetnico/ETrun/wiki/ChangeLog))

Credits
-------

* TJMod developpers
* Racesow project

Bootstrapping
-------------

On *Linux*, make sure you have the following packages installed:

* gcc
* zlib dev package
* libidn dev package

On *OSX*, make sure to have:

* libidn

You can install them via [homebrew](http://mxcl.github.com/homebrew).

Then, you have to get ETrun submodules:

<pre>
$ git submodule init
$ git submodule update
</pre>

If you are running Windows, run `bootstrap.bat`.

Compiling for Windows
---------------------

On Windows: use MSVC compiler.

On Linux, not supported.

Windows server module requires [Microsoft Visual C++ 2010 Redistributable Package](http://www.microsoft.com/en-us/download/details.aspx?id=5555) to run properly.

Compiling for Linux (32-bit)
----------------------------

On Windows: not supported.

On Linux (32-bit only), run:

<pre>
$ ./build.sh
</pre>

Compiling for OSX
-----------------

On OSX, run:

<pre>
$ ./build.sh
</pre>

You can also use Xcode projects located in src directory.
