Duncan Gans
GIS Algorithms
December 18, 2017

slr.cpp
  This file includes the actual functionality of the code. It reads
  a terrain grid, and at the users imput can render what sea level 
  rise of a given height will look like. Furthermore, users can
  insert a range, to see what sea level rises around the initial given
  height can look like. It also creates a asc file that represents the
  new terrain with the sea level rise that can be rendered using the
  render2d code from before. There is no slr.h file because I use no structs
  and it would simply be a few function declarations.
 
README.TXT
  The file you are looking at.

Makefile
  Creates an executable from slr.cpp, as can be seen in slr.o and slr

test1.asc
  A basic test I made to ensure that sea level rise wouldn't magically
  jump to a low elevation without a route. Small file, so the rendering
  looks kinda funny.
 
Problems, qualifications:
  To my knowledge, the code works well. I've checked it on Portland_me.asc,
  Oahu-5m.asc, bremen_dem.asc, brunsdem.asc, and southport_dem.asc, and 
  lincoln-2m.asc For all of these it works well. One exception is large tidal
  areas. Although these are dealt with fine in most cases, when the increment
  of flooding is less than 1, so not an int, it is rendered weird. However, if
  the increment is larger or equal to one foot, it can be 1.0005 feet, it should
  render fine. Beyond this, it works on the files well. The vast majority of time
  is in reading the file and moving it into a renderable file.

How to use
  To run, type:
 ./slr <terrain grid> <result grid> <rise (feet)> <increment> <underwater visibility>
  Use z, x, y, Z, X, Y, to move around the grid
  Use '+' to increase the sea level by the command line increment
  Use '-' to decrease the sea level by the command line increment
