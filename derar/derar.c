#define _X_OPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>

#ifndef WIN32
#define O_BINARY 0
#endif

#include "derar.h"
#include "derar_debug.h"
#include "filename.h"
#include "filetree.h"

/* Header types */
#define HEAD_TYPE_ARCHIVE 0x73
#define HEAD_TYPE_FILE    0x74
#define HEAD_TYPE_END     0x7b

/* Flags */
#define HEAD_FLAG_ADD_SIZE          0x8000

/* Archive header specific flags */
#define HEAD_FLAG_ARCHIVE_VOLUME    0x0001
#define HEAD_FLAG_ARCHIVE_COMMENT   0x0002
#define HEAD_FLAG_ARCHIVE_LOCK      0x0004
#define HEAD_FLAG_ARCHIVE_SOLID     0x0008
#define HEAD_FLAG_ARCHIVE_NEW_NAMES 0x0010
#define HEAD_FLAG_ARCHIVE_AUTHENT   0x0020
#define HEAD_FLAG_ARCHIVE_RECOVERY  0x0040
#define HEAD_FLAG_ARCHIVE_ENCRYPTED 0x0080
#define HEAD_FLAG_ARCHIVE_FIRST     0x0100

/*
   TODO: Is this shit really needed?

   Supposedly, read can sometimes be a bitch and only read some of the data
   we want. Is this so only for sockets though?

   TODO: Check success for every lseek.
 */
static int read_chunk(int fd, void *buf, size_t len)
{
	size_t off = 0;
	ssize_t r = 0;

	while (off < len && (r = read(fd, (char *)buf + off, len - off)) > 0)
		off += r;

	if (len - off > 0)
	{
		derar_error("Unexpected EOF\n");
		errno = EIO;
		return 0;
	}

	if (r < 0)
		return 0;

	return 1;
}

static uint8_t read_block(int fd, uint16_t *flags, uint16_t *size, uint32_t *add_size)
{
	char header[7];
	uint8_t type;

	if (!read_chunk(fd, header, 7))
		return 0;

	/* Determine which type the block has */
	type = *((uint8_t *)&header[2]);

	*flags = *((uint16_t *)&header[3]);
	*size = *((uint16_t *)&header[5]);

	if (*flags & HEAD_FLAG_ADD_SIZE || type == HEAD_TYPE_FILE)
	{	
		if (!read_chunk(fd, add_size, 4))
			return 0;
	}
	else
		*add_size = 0;

	return type;
}

static int read_fileblock(int fd, uint16_t flags, uint16_t size, uint32_t add_size, struct entry *root)
{
	char header[21];

	off_t off = lseek(fd, 0, SEEK_CUR) + size - 4;

	if (!read_chunk(fd, header, 21))
		return 0;

	uint32_t f_unp_size  = *((uint32_t *)&header[ 0]);
/*	uint32_t f_ftime     = *((int32_t  *)&header[ 9]); */
	uint8_t  f_method    = *((uint8_t  *)&header[14]);
	uint16_t f_name_size = *((uint16_t *)&header[15]);
	uint32_t f_attr      = *((uint32_t *)&header[17]);
	char    *f_filename;

	uint64_t partsize = add_size;
	uint64_t filesize = f_unp_size;

	if (f_method != 0x30)
	{
		derar_debug("Skipping compressed file...");
		lseek(fd, size - 25 + partsize, SEEK_CUR);

		return 1;
	}

	/* If 0x100 is set the upper byte of pack_size and unp_size are present */	
	if (flags & 0x100)
	{
		uint32_t upper[2];

		if (!read_chunk(fd, upper, 8))
			return 0;

		partsize |= (uint64_t)(upper[0]) << 32;
		filesize |= (uint64_t)(upper[1]) << 32;
	}

	if ((f_filename = malloc(f_name_size + 1)) == NULL)
	{
		derar_error("malloc failed");
		return 0;
	}

	f_filename[f_name_size] = '\0';

	if (!read_chunk(fd, f_filename, f_name_size))
	{
		derar_error("Filename could not be read");

		free(f_filename);
		return 0;
	}

	/* TODO: Convert f_ftime */

	entry(root, f_filename, S_ISDIR(f_attr) ?
		DERAR_TYPE_DIRECTORY : DERAR_TYPE_FILE, fd, off, partsize);

	/* Skip what we have not read, including the file data */
	lseek(fd, size - 25 - (flags & 0x100 ? 8 : 0) - f_name_size + partsize, SEEK_CUR);

	free(f_filename);

	return 1;
}

struct derar *derar_initialize(const char *archive)
{
	int fd = -1;

	char mark[7];

	struct derar *derar = malloc(sizeof(struct derar));

	/* Initialize root directory entry */
	struct entry *root = insert_entry(NULL, "", DERAR_TYPE_DIRECTORY);

	/* Common header fields */
	uint8_t  type;
	uint16_t flags;
	uint16_t size;
	uint32_t add_size;

	char *path = strdup(archive); /* The current path */
	char *format = NULL;          /* The format string of the nth part */
	int n;                        /* Current part number */

	derar->name       = strdup(basename(path));
	derar->root       = root;
	derar->compressed = 0;
	derar->size       = 0;

	for (;;	path_format(path, format, n++))
	{
		derar_debug("Opening %s\n", path);

		/* Open next file */
		if ((fd = open(path, O_RDONLY | O_BINARY)) < 0)
		{
			if (errno == ENOENT)
			{
				derar_debug("No more parts\n");
				break;
			}
			else
			{
				derar_debug("I/O error\n");
				exit(1);
			}
		}

		/* Read and check mark header */
		if (!read_chunk(fd, mark, 7))
			break;

		if (memcmp("Rar!\x1a\x07", mark, 7) != 0)
		{
			derar_error("No marker header\n");
			break;
		}

		/* Read archive header */
		if (read_block(fd, &flags, &size, &add_size) != HEAD_TYPE_ARCHIVE)
		{
			derar_error("No archive header\n");
			break;
		}

		if (format == NULL)
		{
			if (flags & HEAD_FLAG_ARCHIVE_NEW_NAMES)
				format = path_parse(path, &n, 1);
			else
				format = path_parse(path, &n, 0);

			close(fd);
			continue;
		}

		lseek(fd, size + add_size - 7, SEEK_CUR);

		while ((type = read_block(fd, &flags, &size, &add_size)))
		{
			if (type == HEAD_TYPE_FILE)
			{
				if (!read_fileblock(fd, flags, size - 7, add_size, root))
				{
					derar_error("Failed to read fileblock\n");
					break; /* TODO: Break outer also */
				}
			}
			else if (type == HEAD_TYPE_END)
				break;
			else if (lseek(fd, size + add_size - 7, SEEK_CUR) < 0)
			{
				derar_error("lseek failed\n");
				break;
			}
		}
	}

	if (fd != -1)
		close(fd);

	free(path);
	free(format);

	derar_debug("Parse OK\n");

	return derar;
}

void derar_deinitialize(struct derar *derar)
{
	free_entry(derar->root);
	free(derar->name);
	free(derar);
}
