#!/bin/bash

rm -f movie.gif
convert frame*pgm movie.gif
# Decrease framerate by using the "-delay <value>" flag.

rm -f frame*.pgm
