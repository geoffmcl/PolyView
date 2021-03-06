# PolyView2D 20160622

Update 20160622 - 20141005 - 20140713:

#### History

This is a clone/fork of the original polyview2d. It may or may not work as the online documentation states, but should be very close...

site: https://sites.google.com/site/polyview2d/ - checkout the latest [release](https://github.com/oleg-alexandrov/PolyView/releases), Aug 9, 2015... thank you Oleg Alexandrov...

The first clone/fork appeared as - https://gitlab.com/fgtools/osm2xg/tree/master/polyView2D - ie part of another project... this takes over from that now depreciated source...

Also this clone/fork, like the earlier, is built by cmake. No attempt has been made to keep the qmake *.pro file up-to-date, and thus may not build, especially for Qt5...

#### Licence

See License.txt ... [MIT License Terms](http://en.wikipedia.org/wiki/MIT_License)...

#### Build

To compile PolyView from source, you need CMake installed. See -  http://www.cmake.org/install/

The original version was ported to Qt4, this version to Qt5, so to build and run it you need Qt installed, and the environment setup correctly. The main things in Windows, are the Qt5_DIR correctly set, and the PATH adjusted to add the Qt5 bin directory, and here built using MSVC10, in 64-bits, Win7... 

For Qt5 and other MSVC versions, adjustments need to be made in the build-me.bat files, but hopefully, not too many...
 
With everything setup, change directory to the 'build' folder, and run -

````
build-me.bat - Windows
build-me.sh  - Linux
````

Note the Windows install location is C:\MDOS, which is in my PATH and the linux install location is $HOME/bin, also in my PATH. Manually adjust these locations as **desired**...

And in windows the Qt 64-bit, and MSVC10 vc batch for AMD64 must be available. See the build-me.bat, which uses the `msvc2015_64` for the Qt5 binaries... adjustments will be required by other MSVC versions... Qt5 installs...

But even without these scripts the simple build instruction are -

Windows: In the `build` folder run

````
> cmake .. [-DCMAKE_INSTALL_PREFIX=path/for/install]
> cmake --build . --config Release
And if install desired - did you **fix** the install path?
> cmake --build . --config Release --target INSTALL
````

Linux: In the `build` folder run

````
$ cmake .. [-DCMAKE_INSTALL_PREFIX=path/for/install]
$ make
And if install desired
$ make install
````

Running the polyView2D binary, `MENU -> Help -> Show documentation` puts up a complete help window... ie --help outputs a brief help to the console, cout...

The `testData` folder contains lots of example xg files to view. The source indicates it also supports files of (type == "pol" ||type == "cnt"), but this has **NOT** been tested in this source...

The xg file format is a sub-set of xgraph files. polyView2D supports lines like -

````
# this is a comment line, and skipped
anno lon lat Annotation text
color red # trailing comment
lon1 lat1 [; 1:1]
lon2 lat2
lon3 lat3
next
...
````

A very simply and quick method to 'draw' and 'view' a polygon or poly line... in 2D...

Have fun...

Geoff.

Original Text 20111109: - DEPRECIATED - NOT TESTED
-----------------------

As stated above no attempt has been made to keep the .pro file
correct, so this may fail...

PolyView is a free polygon viewer. The PolyView documentation is at

https://sites.google.com/site/polyview2d/

To compile PolyView from source, the Qt 4 development libraries need to
be installed. They can can be obtained in Ubuntu by typing

````
$ [sudo] apt-get install qmake
````

The next steps are:

````
cd gui
qmake polyview.pro 
make
````

The 'qmake' command will generate a makefile which whill be used by
'make' to build the executable. The 'gui' directory has a sample
makefile as generated by 'qmake'.

As stated, **not** tested, ...

; eof
