ETrun
=====

ETrun is a Wolfenstein: Enemy Territory game modification based on ET-GPL.
The objective of this mod is to bring timeruns support to it.
ETrun is currently under development.

**Visit www.timeruns.net for more information.**

Key features
============

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
=======

* TJMod developpers
* Racesow project

Compatibility
=============

Windows
-------

Windows server module `qagame_mp_x86.dll` requires [Microsoft Visual C++ 2010 Redistributable Package](http://www.microsoft.com/en-us/download/details.aspx?id=5555) to run properly.
Works (tested) on `XP/7` with `ET 2.60b`.

Linux
-----

Requires `GLIBC >= 2.7`.
Works (tested) on `Debian 6`, `Linux Mint 12` with `ET 2.60b` and `ET: Legacy`.

OSX
---

Not compatible with `ET 2.60d`.
Works (tested) on `OSX 10.8` with `ET: Legacy.

Bootstrapping
=============

Get ETrun submodules:

<pre>
$ git submodule init
$ git submodule update
</pre>

Linux / OSX
-----------

You will need `cmake` to compile this project.

Windows
-------

You need Microsoft Visual studio 2010 or newer.

Compiling
=========

Host refers to the OS used to compile.
Target refers to the OS where the game will be executed.

<table>
	<tr>
		<th>Target \ Host</th>
		<th>Windows</th>
		<th>Linux (32-bit only)</th>
		<th>OSX</th>
	</tr>
	<tr>
		<th>Windows</th>
		<td>Use ETrun.sln with MSVC</td>
		<td>Not supported</td>
		<td>Not supported</td>
	</tr>
	<tr>
		<th>Linux</th>
		<td>Not supported</td>
		<td>./build.sh</td>
		<td>Not supported</td>
	</tr>
	<tr>
		<th>OSX</th>
		<td>Not supported</td>
		<td>Not supported</td>
		<td>./build.sh or Xcode projets</td>
	</tr>
</table>
