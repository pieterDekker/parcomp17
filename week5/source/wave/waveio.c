#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "waveio.h"

void stretchContrast (real ***u, int N, int NFRAMES)
{
	int i, j, frame;
	real min, max, scale = 255.0;
	
	min = 9999;
	max = -9999;
	for (frame = 2; frame < NFRAMES; frame++)
	{
		for (i = 0; i < N; i++)
		{
			for (j = 0; j < N; j++)
			{
				if (u[frame][i][j] < min)
					min = u[frame][i][j];
				if (u[frame][i][j] > max)
					max = u[frame][i][j];
			}
		}
	}
	if (max > min)
		scale = scale / (max - min);
	for (frame = 0; frame < NFRAMES; frame++)
		for (i = 0; i < N; i++)
			for (j = 0; j < N; j++)
				u[frame][i][j] = scale * (u[frame][i][j] - min);
}

void saveFrames (real ***u, int N, int NFRAMES, int bw)
{
	FILE *ppmfile;
	char filename[32];
	int i, j, k, frame, idx;
	byte lut[256][3]; // Colour lookup table
	byte *rgb; // Framebuffer

	if (bw)
		// Greyscale
		for (i=0; i<256; i++)
			lut[i][0] = lut[i][1] = lut[i][2] = (byte)i;
	else
	{
		// Colour
		for (i = 0; i < 256; i++)
		{
			lut[i][0] = (byte)i;
			lut[i][1] = (byte)(127 + 2 * (i < 64 ? i : (i < 192 ? 128 - i : i - 255)));
			lut[i][2] = (byte)(255 - i);
		}
	}

	rgb = malloc (3 * N * N * sizeof (byte));
	for (frame = 0; frame < NFRAMES; frame++)
	{
		k = 0;
		for (i = 0; i < N; i++)
		{
			for (j = 0; j < N; j++)
			{
				idx = (int)u[frame][i][j];
				rgb[k++] = (byte)lut[idx][0];  // Red
				rgb[k++] = (byte)lut[idx][1];  // Green
				rgb[k++] = (byte)lut[idx][2];  // Blue
			}
		}

		// Show colour map (comment this out if you don't like it).
		if (N >= 255)
		{
			int i0 = N / 2 - 128;
			for (i = 0; i < 256; i++)
			{
				k = 3 * ((i + i0) * N + 10);
				for (j = 0; j < 16; j++)
				{
					rgb[k++] = lut[i][0];
					rgb[k++] = lut[i][1];
					rgb[k++] = lut[i][2];
				}
			}
		}
		sprintf (filename, "frame%04d.ppm", frame);
		ppmfile = fopen (filename, "wb");
		fprintf (ppmfile, "P6\n");
		fprintf (ppmfile, "%d %d\n", N, N);
		fprintf (ppmfile, "255\n");
		fwrite (rgb, sizeof (byte), 3 * N * N, ppmfile);
		fclose (ppmfile);
	}
	free (rgb);
}

void parseIntOpt (int argc, char **argv, char *str, int *val)
{
	int i, found=0;
	for (i = 1; i < argc; i++) {
		if (strcmp (argv[i], str) == 0)
		{
			found++;
			if (found > 1)
			{
				fprintf (stderr, "Error: doubly defined option %s\n", str);
				fflush (stderr);
				exit (EXIT_FAILURE);
			}
			if (i + 1 < argc)
			{
				*val = atoi (argv[i + 1]);
			} else
			{
				fprintf (stderr, "Error: missing numeric  value after %s\n", str);
				fflush (stderr);
				exit (EXIT_FAILURE);
			}
		}
	}
}

void parseRealOpt (int argc, char **argv, char *str, real *val)
{
	int i, found=0;
	for (i = 1; i < argc; i++)
	{
		if (strcmp (argv[i], str) == 0)
		{
			found++;
			if (found > 1)
			{
				fprintf (stderr, "Error: doubly defined option %s\n", str);
				fflush (stderr);
				exit (EXIT_FAILURE);
			}
			if (i + 1 < argc)
			{
				*val = (real)atof (argv[i + 1]);
			} else
			{
				fprintf (stderr, "Error: missing numeric  value after %s\n", str);
				fflush (stderr);
				exit (EXIT_FAILURE);
			}
		}
	}
}
