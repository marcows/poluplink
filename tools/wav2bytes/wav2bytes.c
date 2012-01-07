/*
 * Converts a Polar UpLink wav file to a file with the serial protocol byte
 * stream.
 *
 * These files are of the following format and only this is supported:
 * RIFF (little-endian) data, WAVE audio, Microsoft PCM, 8 bit, mono 44100 Hz
 *
 * Files exported from Polar UpLink Tool should work perfectly.
 *
 * Files recorded from playback may cause problems because of noise depending
 * on the signal level. If the conversion goes wrong (CRC verification fails),
 * you can tweak the noise tolerance or the minimum count of nonzerocount in
 * the source. Also the conversion is not yet flexible: the file has to begin
 * directly with the start bit. Trailing silence causes errors "start bit not
 * logical 1", but the concerned bytes ignored, so this does not hurt.
 *
 * Copyright (C) 2012 Markus Heidelberg <markus.heidelberg@web.de>
 *
 * Based on examples/sndfile-to-text.c from libsndfile-1.0.25
 *
 *
** Copyright (C) 2008-2011 Erik de Castro Lopo <erikd@mega-nerd.com>
**
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in
**       the documentation and/or other materials provided with the
**       distribution.
**     * Neither the author nor the names of any contributors may be used
**       to endorse or promote products derived from this software without
**       specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
** TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
** PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
** CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
** OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
** OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
** ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sndfile.h>

#define	BLOCK_SIZE (11*441) // 441 is the number of sample values per bit/symbol of the serial protocol
			    // 11 is the number of bits per frame/byte of the serial protocol

void crc_process
( unsigned short * context,
  unsigned char ch )
{
  unsigned short uch  = (unsigned short) ch;
  int i;

  *context ^= ( uch << 8 );

  for ( i = 0; i < 8; i++ )
    {
      if ( *context & 0x8000 )
        *context = ( *context << 1 ) ^ 0x8005;
      else
        *context <<= 1;
    }
}

void crc_block
( unsigned short * context,
  const unsigned char * blk,
  int len )
{
  while ( len -- > 0 )
    crc_process ( context, * blk ++ );
}

static void
print_usage (char *progname)
{	printf ("\nUsage : %s <input file> <output file>\n", progname) ;
	puts ("\n"
		"    Where the output file will contain a line for each frame/byte (text file)\n"
		"    or a byte for each frame/byte (binary file) of the serial protocol.\n"
		) ;

} /* print_usage */

static void
convert_to_text (SNDFILE * infile, FILE * outfile)
{	int buf [BLOCK_SIZE] ;
	int serialbit, k, readcount, nonzerocount ;
	short serialframe, serialbyte;
	int serialframecount = 0;
	int serialframeerrors = 0;

	unsigned short crc = 0;

	while ((readcount = sf_read_int (infile, buf, BLOCK_SIZE)) > 0) {
		serialframe = 0;
		for (serialbit = 0; serialbit < 11; serialbit++) {
			nonzerocount = 0;
			for (k = 0 ; k < readcount/11 ; k++) {
				unsigned char sampleval = ((unsigned int)buf [serialbit * 441 + k] >> 24) ^ 0x80;
				if (sampleval < 0x80-0x3 || sampleval > 0x80+0x3) {
					if (k < readcount/11 / 2)
						nonzerocount++;
					else
						fprintf (stderr, "warning: RZ code in serial frame %2d bit %2d: "
								"waveform should be silent (about 0x80 with tolerance), instead: 0x%02X\n",
								serialframecount, serialbit, sampleval);
				}
				//fprintf (stderr, "%8u: %02x %d\n", serialbit * 441 + k, sampleval, nonzerocount);
			}
			if (nonzerocount > readcount/11 / 3)
				serialframe |= (1 << serialbit);
		}
		if (!(serialframe & (1 << 0))) {
			serialframeerrors++;
			fprintf (stderr, "error in serial frame %2d: start bit not logical 1\n", serialframecount);
		}
		if ((serialframe & (1 << 9))) {
			serialframeerrors++;
			fprintf (stderr, "error in serial frame %2d: stop bit #1 not logical 0\n", serialframecount);
		}
		if ((serialframe & (1 << 10))) {
			serialframeerrors++;
			fprintf (stderr, "error in serial frame %2d: stop bit #2 not logical 0\n", serialframecount);
		}

		// 8 bits payload, LSB first
		serialbyte = (serialframe >> 1) & 0xFF;

		if (serialframeerrors == 0) {
			// text file: one byte per line
			fprintf (outfile, "%02X\n", serialbyte);
			// binary file, file has to be opened binary to be portable
			//fputc (serialbyte, outfile);

			crc_process (&crc, serialbyte);
		} else {
			fprintf (stderr, "warning: byte with value 0x%02X of serial frame %2d ignored because of %d logical errors\n",
					serialbyte, serialframecount, serialframeerrors);
		}

		serialframecount++;
		serialframeerrors = 0;
	}

	if (crc != 0)
		fprintf (stderr, "error in CRC: 0x%04X instead of 0\n", crc);

	return ;
} /* convert_to_text */

int
main (int argc, char * argv [])
{	char 		*progname, *infilename, *outfilename ;
	SNDFILE	 	*infile = NULL ;
	FILE		*outfile = NULL ;
	SF_INFO	 	sfinfo ;

	progname = strrchr (argv [0], '/') ;
	progname = progname ? progname + 1 : argv [0] ;

	if (argc != 3)
	{	print_usage (progname) ;
		return 1 ;
		} ;

	infilename = argv [1] ;
	outfilename = argv [2] ;

	if (strcmp (infilename, outfilename) == 0)
	{	printf ("Error : Input and output filenames are the same.\n\n") ;
		print_usage (progname) ;
		return 1 ;
		} ;

	if (infilename [0] == '-')
	{	printf ("Error : Input filename (%s) looks like an option.\n\n", infilename) ;
		print_usage (progname) ;
		return 1 ;
		} ;

	if (outfilename [0] == '-')
	{	printf ("Error : Output filename (%s) looks like an option.\n\n", outfilename) ;
		print_usage (progname) ;
		return 1 ;
		} ;

	if ((infile = sf_open (infilename, SFM_READ, &sfinfo)) == NULL)
	{	printf ("Not able to open input file %s.\n", infilename) ;
		puts (sf_strerror (NULL)) ;
		return 1 ;
		} ;

	/* Open the output file. */
	if ((outfile = fopen (outfilename, "w")) == NULL)
	{	printf ("Not able to open output file %s : %s\n", outfilename, sf_strerror (NULL)) ;
		return 1 ;
		} ;

	convert_to_text (infile, outfile) ;

	sf_close (infile) ;
	fclose (outfile) ;

	return 0 ;
} /* main */

