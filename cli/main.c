#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <poluplink.h>

int main(int argc, char *argv[])
{
	int ret = 0;

	size_t rawbytes_read = 0;
	size_t rawbytes_written;

	uint8_t *rawbytes;
	long txtinsize;

	FILE *txtin, *wavout;

	if (argc != 3) {
		printf("Usage: %s <txt input file> <wav output file>\n", argv[0]);
		return 1;
	}

	if (strcmp(argv[1], argv[2]) == 0) {
		printf ("Error: input and output filenames are the same\n");
		return 1 ;
	}

	txtin = fopen(argv[1], "r");
	if (txtin == NULL) {
		printf("Error: could not open input file %s\n", argv[1]);
		return 1;
	}

	wavout = fopen(argv[2], "wb");
	if (wavout == NULL) {
		printf("Error: could not open output file %s\n", argv[2]);
		fclose(txtin);
		return 1;
	}

	/* read txt */
	fseek(txtin, 0, SEEK_END);
	txtinsize = ftell(txtin);
	fseek(txtin, 0, SEEK_SET);

	// allocate more than needed, but it is a safe value that cannot be exceeded
	rawbytes = (uint8_t *)malloc(txtinsize);

	while (!feof(txtin)) {
		unsigned char s[5];

		// one line per raw byte, encoded as hexadecimal string
		if (fgets (s, sizeof(s), txtin) != NULL)
		{
			sscanf(s, "%x", rawbytes + rawbytes_read);
			rawbytes_read++;
		}
	}

	/* write wav */
	rawbytes_written = poluplink_export_wav(wavout, rawbytes, rawbytes_read);

	if (rawbytes_written != rawbytes_read) {
		printf("Could not write file completely, only %lu of %lu serial bytes written.\n",
				(unsigned long)rawbytes_written, (unsigned long)rawbytes_read);
		ret = 1;
	}

	fclose(wavout);
	fclose(txtin);

	return ret;
}
