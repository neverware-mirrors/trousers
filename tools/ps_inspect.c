/*
 *
 *   Copyright (C) International Business Machines  Corp., 2005
 *
 *   This program is free software;  you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY;  without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *   the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program;  if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/*
 * ps_inspect.c
 *
 *   Inspect a persistent storage file, printing information about it based
 *   on best guesses.
 *
 *   There are 2 different types of persistent storage files:
 *
 * A)
 *
 * [UINT32   num_keys_on_disk]
 * [TSS_UUID uuid0           ]
 * [TSS_UUID uuid_parent0    ]
 * [UINT16   pub_data_size0  ]
 * [UINT16   blob_size0      ]
 * [UINT16   cache_flags0    ]
 * [BYTE[]   pub_data0       ]
 * [BYTE[]   blob0           ]
 * [...]
 *
 * B)
 *
 * [BYTE     TrouSerS PS version='1']
 * [UINT32   num_keys_on_disk       ]
 * [TSS_UUID uuid0                  ]
 * [TSS_UUID uuid_parent0           ]
 * [UINT16   pub_data_size0         ]
 * [UINT16   blob_size0             ]
 * [UINT32   vendor_data_size0      ]
 * [UINT16   cache_flags0           ]
 * [BYTE[]   pub_data0              ]
 * [BYTE[]   blob0                  ]
 * [BYTE[]   vendor_data0           ]
 * [...]
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <trousers/tss.h>

#define PRINTERR(...)	fprintf(stderr, ##__VA_ARGS__)
#define PRINT(...)	printf("PS " __VA_ARGS__)

/* one global buffer we read into from the PS file */
unsigned char buf[1024];

void
usage(char *argv0)
{
	PRINTERR("usage: %s filename\n", argv0);
	exit(-1);
}

void
print_hex(BYTE *buf, UINT32 len)
{
	UINT32 i = 0, j;

	while (i < len) {
		for (j=0; (j < 4) && (i < len); j++, i+=4)
			printf("%02x%02x%02x%02x ",
				buf[i] & 0xff, buf[i+1] & 0xff,
				buf[i+2] & 0xff, buf[i+3] & 0xff);
		printf("\n");
	}
}


int
printkey_0(int i, FILE *f)
{
	UINT16 pub_data_size, blob_size, cache_flags;
	int members;

	PRINT("uuid%d: ", i);
	print_hex(buf, sizeof(TSS_UUID));

	PRINT("parent uuid%d: ", i);
	print_hex(&buf[sizeof(TSS_UUID)], sizeof(TSS_UUID));

	pub_data_size = *(UINT16 *)&buf[(2 * sizeof(TSS_UUID))];
	blob_size = *(UINT16 *)&buf[(2 * sizeof(TSS_UUID)) + sizeof(UINT16)];
	cache_flags = *(UINT16 *)&buf[2*sizeof(TSS_UUID) + 2*sizeof(UINT16)];

	PRINT("pub_data_size%d: %hu\n", i, pub_data_size);
	PRINT("blob_size%d: %hu\n", i, blob_size);
	PRINT("cache_flags%d: %02hx\n", i, cache_flags);

	/* trash buf, we've got what we needed from it */
	if ((members = fread(buf, pub_data_size + blob_size,
			1, f)) != 1) {
		PRINTERR("fread: %s: %d members read\n", strerror(errno), members);
		return -1;
	}

	PRINT("pub_data%d:\n", i);
	print_hex(buf, pub_data_size);

	PRINT("blob%d:\n", i);
	print_hex(&buf[pub_data_size], blob_size);

	return 0;
}

int
printkey_1(FILE *f)
{
	return 0;
}

int
version_0_print(FILE *f)
{
	int rc, members = 0;
	UINT32 i, u32 = *(UINT32 *)buf;

	PRINT("version:        0\n");
	PRINT("number of keys: %u\n", u32);

	/* The +- 1's below account for the byte we read in to determine
	 * if the PS file had a version byte at the beginning */

	/* align the beginning of the buffer with the beginning of the key */
	memcpy(buf, &buf[4], sizeof(TSS_UUID) + 1);

	/* read in the rest of the first key's header */
	if ((members = fread(&buf[sizeof(TSS_UUID) + 1],
			sizeof(TSS_UUID) + (3 * sizeof(UINT16)) - 1,
			1, f)) != 1) {
		PRINTERR("fread: %s\n", strerror(errno));
		return -1;
	}

	if (printkey_0(0, f)) {
		PRINTERR("printkey_0 failed.\n");
		return -1;
	}

	for (i = 1; i < u32; i++) {
		/* read in subsequent key's headers */
		if ((members = fread(buf, 2*sizeof(TSS_UUID) + 3*sizeof(UINT16),
					1, f)) != 1) {
			PRINTERR("fread: %s\n", strerror(errno));
			return -1;
		}

		if ((rc = printkey_0(i, f)))
			return rc;
	}

	return 0;
}

int
version_1_print(FILE *f)
{
	int rc;
	UINT32 i, *u32 = (UINT32 *)&buf[1];

	PRINT("version:        1\n");
	PRINT("number of keys: %u\n", *u32);

	for (i = 0; i < (*u32 - 1); i++) {
		if ((rc = printkey_1(f)))
			return rc;
	}

	return 0;
}

int
inspect(FILE *f)
{
	int members = 0;
	UINT32 *num_keys;
	TSS_UUID SRK_UUID = TSS_UUID_SRK;

	/* do the initial read, which should include sizeof(TSS_UUID)
	 * + sizeof(UINT32) + 1 bytes */
	if ((members = fread(buf,
			sizeof(TSS_UUID) + sizeof(UINT32) + 1,
			1, f)) != 1) {
		PRINTERR("fread: %s\n", strerror(errno));
		return -1;
	}

	if (buf[0] == '\1') {
		num_keys = (UINT32 *)&buf[1];
		if (*num_keys == 0)
			goto version0;

		if (memcmp(&buf[5], &SRK_UUID, sizeof(TSS_UUID)))
			goto version0;

		return version_1_print(f);
	}

version0:
	return version_0_print(f);
}

int
main(int argc, char ** argv)
{
	FILE *f = NULL;
	int rc;

	if (argc != 2)
		usage(argv[0]);

	if ((f = fopen(argv[1], "r")) == NULL) {
		PRINTERR("fopen(%s): %s\n", argv[1], strerror(errno));
		return -1;
	}

	PRINT("filename: %s\n", argv[1]);

	rc = inspect(f);

	fclose(f);

	return rc;
}
