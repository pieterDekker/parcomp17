
#ifndef WAVEIO_H
#define WAVEIO_H

typedef float real;
typedef unsigned char byte;

void stretchContrast (real ***u, int N, int NFRAMES);
void saveFrames (real ***u, int N, int NFRAMES, int bw);
void parseIntOpt (int argc, char **argv, char *str, int *val);
void parseRealOpt (int argc, char **argv, char *str, real *val);

#endif
