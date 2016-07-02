# PolyView Fork

This is a **fork** of https://github.com/oleg-alexandrov/PolyView.

The first aim was a clean build in Windows using CMake and MSVC. That is in the `master` branch.

Then the `Qt5` branch contains a port to **Qt5**. Seems fairly complete, but may need some adjustments for the various versions of MSVC and MinGW.

![ScreenShot](gui/pvLogo.png)

PolyView is a free and open source cross-platform program designed to
quickly load, view, and edit multiple files containing polygons. It
can zoom and pan, show polygons as edges, points, and filled, display
text labels, print the coordinates of vertices and measure distances,
change the order in which polygons are displayed, choose which
polygons to show, etc.

PolyView can add, delete, move, rotate, and scale polygons, add,
delete, and move vertices and edges, add and delete text labels, and
cut polygons to a box.

# Download

No binary releases have yet been released by this fork, but the parent repository had one, circa Aug 9, 2015 -

* https://github.com/oleg-alexandrov/PolyView/releases

On Linux and OSX the PolyView program is installed in the *bin*
directory.  On Windows it is in the base directory. The Windows
version is large because of the size of the Qt libraries.

# License

PolyView is available under the MIT license and can be used for any
purpose, academic or commercial.

# Compiling

PolyView is written in C++. It was successfully compiled on Linux with
g++, on OSX with clang, and on Windows with MinGW and MSVC, in both 32 and 
64-bit versions. 

It was written with portability in mind and it should be possible to compile it on any platform and compiler.

Its only dependency is the Qt headers and libraries. THe original version uses Qt4 (e.g., Qt 4.8), but here have started a Qt5 (e.g., Qt 5.6). See `Qt5` branch... 

Instructions on how to compile Qt are given at the end of this document.

In this fork CMake configuration and generation has **replaced** the `QMake` build.

To compile PolyView on any system run:

```
$ cd build
$ cmake ..
$ cmake --build . --config Release
```

While the original polyview.pro has been left in place, it has **not** been kept up-to-date, **and** has not been ported to Qt5.

# Comparison with XGRAPH

PolyView was inspired by XGRAPH (http://www.xgraph.org/). The latter
is a general purpose data plotter and has a lot of features PolyView
lacks. PolyView has extra features for viewing and editing polygons.

PolyView is (as of 2007) more responsive for polygons with very many 
vertices, and it can handle zooming to small regions of polygons with 
large floating point vertex coordinates without overflowing and 
showing incorrect results. Credit for responsiveness goes to Qt,
and issues with overlow required careful handling.

Lastly, PolyView is open-source and under a liberal license, and can
be improved in a collaborative manner.

# Documentation

## File format

PolyView uses the XGRAPH file format, in which the x and y coordinates of each
polygon are stored as two columns in a plain text file followed by the
"NEXT" statement. Here is a sample polygon file storing a rectangle
and a triangle.

```
color = red
6.5 5.0
7.2 5.0
7.2 7.0
6.5 7.0
6.5 5.0
NEXT
color = green
8.0 3.0
9.0 3.0
8.0 4.0
8.0 3.0
NEXT
anno 7.0 5.5 text label
```

Polygon vertices are stored in double precision.

Notice that the first vertex of each polygon is repeated again before
the "NEXT" statement, to signal to the viewer that the polygon is
closed. PolyView can also display polygonal lines, in addition to
polygons, when the last vertex need not equal the first one. The last line
above shows how to place a text label, with its coordinates and text.


## Functionality

### Mouse buttons

The left mouse button snaps to the closest polygon vertex and prints
its coordinates in the terminal used to start PolyView. A subsequent
click also prints the distance from the previous vertex to the current
one.  The middle mouse button prints the coordinates of where the
mouse clicked, without snapping to the closest vertex.  Dragging the
mouse from lower-left to upper-right zooms in, and doing it in reverse
zooms out.  Dragging the mouse while keeping the Control key pressed
creates a highlight which can be used to cut the polygons to the
highlight or to paste/move/delete them.

### Keyboard shortcuts

Panning is accomplished by using the arrow keys, zooming is done with
the plus and minus keys. Many other actions are bound to keyboard
keys, as can be seen when invoking them from the menu.

### Command box

Many GUI operations (such as zoom) echo their action as a command in
the terminal. That command can be pasted in the command box at the
bottom of the PolyView GUI to reproduce the original operation. This
provides a basic level of scripting and reproducibility.

### Menu functions 

#### File menu

* Load a polygon file in addition to existing files
* Save the polygons as one file
* Save the polygons as individual files
* Overwrite the existing polygons
* Save a png image file

#### View menu

* Choose which files to hide/show
* Zoom and pan
* Reset the view to contain all polygons with a small padding
* Change the order in which the polygons are displayed
* Show/hide annotations (text labels)
* Show the polygons filled with color
* Show the vertices only (no edges)
* Show the index of each vertex
* Show the layer ids (if present)

#### Edit menu

* Undo and redo
* Create a polygon with integer vertices and edge angles multiple of 45 degrees
* Enforce integer vertices and edge angles multiple of 45 degrees
* Create a polygon with arbitrary angles

#### Transform menu

* Translate/rotate/scale the polygons

#### Selection menu

* Create a highlight (the polygons in the highlight are automatically selected and copied to a buffer)
* Cut the polygons to the current highlight
* Delete the selected polygons
* Paste the selected polygons
* Move the selected polygons (use Shift-Mouse)
* Deselect all polygons and delete all highlights
* Translate/rotate/scale/transform selected polygons

#### Grid menu

* Show/hide the grid
* Enforce that all polygon edges have angles multiple of 45 degrees and snap the vertices to the grid
* Set the grid size
* Set the grid linewidth
* Set the grid color

#### Diff menu

* Change the colors of polygons so that polygons from different files have different colors
* Enter diff mode (a mode in which two similar polygon files can be compared)
* Show the next/previous difference between given two given polygon files (starting with the largest difference)

#### Options menu

* Set the linewidth of polygon edges
* Set the background color

#### Right-click menu

* Show and save a mark at the current point
* Create a polygon with integer vertices and edge angles multiple of 45 degrees
* Enforce integer vertices and edge angles multiple of 45 degrees
* Create a polygon with arbitrary angles
* Delete the polygon at mouse cursor
* Enter move polygons mode (use Shift-Mouse to move a polygon; if some polygons are selected using a highlight, then all selected polygons will be moved)
* Enter move vertices mode (use Shift-Mouse to move a vertex)
* Enter move edges mode (use Shift-Mouse to move an edge)
* Insert a vertex on the edge closest to the mouse cursor
* Delete the vertex closest to the mouse cursor
* Copy the polygon closest to the mouse cursor
* Paste the polygon closest to the mouse cursor
* Reverse the orientation of the polygon closest to the mouse cursor
* Insert text label
* Delete text label
* Enter align mode (a mode in which, given two polygon files, the second polygon file is kept fixed, while the first one can be interactively translated using Shift-Mouse and rotated/flipped from the right-click menu until it aligns to the second one)

# Command line options

PolyView will open simultaneously all polygon files supplied as inputs
on the command line. Various command line options can modify how the
polygons are displayed.

* -h | -help	Show the available command line options
* -geo[metry] `width`x`height`	The window size in pixels (for example, 800x800)
* -bg | -backgroundColor `color`	Color of the background (the default is black)
* -c | -color `color`	All polygons after this option will show up in the given color (the default is to use the colors specified in the polygon files) 
* -fs | -fontSize `integer`	The text font size in pixels
* -lw | -lineWidth `integer`	All polygons after this option will show up with given linewidth
* -p | -points	All polygons after this option will show up as vertices rather than edges (a subsequent -p option undoes this behavior)
* -cp | -closedPoly	All polygons after this option will show up as closed (the last vertex is connected to the first one)
* -nc | -nonClosedPoly	Interpret the polygons after this option as polygonal lines (the last vertex is not connected to the first one)
* -f | -filledPoly	All polygons after this option will show up as filled
* -nf | -nonFilledPoly	All polygons after this option will show up as not filled
* -grid on | off	Turn on/off the grid display
* -gridSize `integer`	Grid size
* -gridWidth `integer`	Grid width in pixels
* -gridColor `color`	Grid color

Note, certain options are saved to a persistent INI file.

# Compiling Qt 

PolyView was tested to compile on Linux, OSX, and Windows with Qt 4.8 but other 4.x versions should work as well, including Qt5...

Qt can be installed on Ubuntu with the command:

```
apt-get install qmake
```

To compile Qt 4.8 from source, get the source code from:

*  https://download.qt.io/archive/qt/4.8/

After unzipping the archive and ensuring that the compiler is in the
path, run:

* configure -opensource -fast -confirm-license -nomake demos -nomake examples -nomake docs -nomake translations -no-webkit -no-script -no-scripttools -no-openssl -no-libjpeg -no-libmng  -no-libtiff -no-cups -no-nis -no-opengl -no-openvg -no-phonon -no-phonon-backend -no-sql-psql -no-dbus

# Author

Author: Oleg Alexandrov (oleg.alexandrov@gmail.com)
Contibutor: Geoff R. McLane (reports@geoffair.info)

; eof - 20160702
