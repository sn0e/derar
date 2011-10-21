#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#ifdef WIN32
#define PATH_DELIMITER "\\"
#define WIN32_LEAN_AND_MEAN
#define WINVER 0x501
#include <windows.h>
#include "strtok_r.h"
#else
#define PATH_DELIMITER "/"
#endif

#include "derar.h"
#include "derar_types.h"	

#define MIN(a, b) (a < b ? a : b)

const char *derar_name(struct derar *derar)
{
	return derar->name;
}

uint64_t derar_total_size(struct derar *derar)
{
	return derar->size;
}

struct derar_handle *derar_open(struct derar *derar, const char *path)
{
	struct entry *ent0, *ent = derar->root;
	char *str, *saveptr = NULL;
	char *path_copy;

	if (path != NULL && path[0] != '\0')
	{
		path_copy = strdup(path);

		for (str = strtok_r(path_copy, PATH_DELIMITER, &saveptr);
			 str != NULL;
			 str = strtok_r(NULL, PATH_DELIMITER, &saveptr))
		{
			for (ent0 = ent->dir.subentry; ent0; ent0 = ent0->next)
				if (strcmp(ent0->name, str) == 0)
					break;

			if (ent0)
				ent = ent0;
			else
			{
				free(path_copy);

				errno = ENOENT;
				return NULL;
			}
		}

		free(path_copy);
	}

	struct derar_handle *handle;

	/* Allocate the handle structure */
	if ((handle = malloc(sizeof(struct derar_handle))) == NULL)
		return NULL; /* errno set by malloc */

	handle->entry = ent;

	/* Reset handle state */
	if (ent->type == DERAR_TYPE_DIRECTORY)
		handle->dir.current = ent->dir.subentry;
	else
	{
		handle->file.part = ent->file.part;
		handle->file.off  = 0;
	}

	return handle;
}

void derar_close(struct derar_handle *handle)
{
	free(handle);
}

static int part_cmp(const void *p1, const void *p2)
{
	uint64_t off = *(uint64_t *)p1;
	struct part *part = (struct part *)p2;

	if (off >= part->foff && off < part->foff + part->size)
		return 0;
	else if (off < part->foff)
		return -1;
	else
		return 1;
}

int derar_rewinddir(struct derar_handle *handle)
{
	if (handle->entry->type == DERAR_TYPE_FILE)
	{
		errno = ENOTDIR;
		return -1;
	}

	handle->dir.current = handle->entry->dir.subentry;

	return 0;
}

int derar_readdir(struct derar_handle *handle, const char **name, enum derar_type *type, time_t *mtime, uint64_t *size)
{
	if (handle->entry->type == DERAR_TYPE_FILE)
	{
		errno = ENOTDIR;
		return -1;
	}

	if (handle->dir.current != NULL)
	{
		*name  = handle->dir.current->name;
		*type  = handle->dir.current->type;
		*mtime = time(NULL);

		if (handle->dir.current->type == DERAR_TYPE_FILE)
			*size = handle->dir.current->file.size;
		else
			*size = 0;

		handle->dir.current = handle->dir.current->next;

		return 1;
	}

	return 0;
}

ssize_t derar_read(struct derar_handle *handle, void *buf, size_t nbyte, uint64_t offset)
{
	/* TODO: Make thread-safe (using dup?) */

	struct entry *entry = handle->entry;
	struct part *part;
	uint64_t off = 0;

	if (handle == NULL)
	{
		fputs("derar_read: EINVAL\n", stderr);

		errno = EINVAL;
		return -1;
	}

	/* Can't read from a directory! */
	if (entry->type == DERAR_TYPE_DIRECTORY)
	{
		fputs("derar_read: EISDIR\n", stderr);

		errno = EISDIR;
		return -1;
	}

	if (nbyte == 0)
		return 0;

	/* The file handle has a current part pointer, see if it is the right one */
	if (part_cmp(&offset, handle->file.part) == 0)
		part = handle->file.part;
	else
	{
		part = bsearch(&offset, entry->file.part, entry->file.parts, sizeof(struct part), part_cmp);

		/* The offset was past EOF so nothing to read */
		if (part == NULL)
			return 0;

		handle->file.part = part;
	}

	while (nbyte - off > 0)
	{
		off_t poff = offset + off - part->foff;
		size_t nread = MIN(nbyte - off, part->size - poff);

#ifndef WIN32
		if (pread(part->fd, buf + off, nread, poff + part->off) < 0)
		{
			perror("pread");
			return -1;
		}
#else
		uint64_t Offset = poff + part->off;

		DWORD lpNumberOfBytesRead;
		OVERLAPPED ol;

		memset(&ol, '\0', sizeof(ol));
		ol.Offset = (DWORD)(Offset);
		ol.OffsetHigh = (DWORD)(Offset >> 32);

		if (ReadFile(part->handle, buf + off, nread, &lpNumberOfBytesRead, &ol) == FALSE)
		{
			fprintf(stderr, "ReadFile: %d\n", (int)GetLastError());
			return -1;
		}

		nread = lpNumberOfBytesRead;

#endif

		/* We successfully read nread bytes */
		off += nread;

		/* Stop reading if we are at EOF */
		if (offset + off >= entry->file.size)
			break;

		if (offset + off >= part->foff + part->size)
			part++;
	}

	handle->file.part = part;

	return off;
}

