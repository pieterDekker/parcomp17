#!/bin/bash

rm -f movie.gif
convert frame*ppm movie.gif
# Decrease framerate by using the "-delay <value>" flag.

rm -f frame*.ppm
