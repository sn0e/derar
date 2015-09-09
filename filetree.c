#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "derar.h"
#include "derar_types.h"

#ifdef WIN32
#include "strtok_r.h"
#endif

enum derar_type derar_type(struct derar_handle *handle)
{
	return handle->entry->type;
}

uint64_t derar_size(struct derar_handle *handle)
{
	if (handle->entry->type == DERAR_TYPE_FILE)
		return handle->entry->file.size;

	return 0;
}

void free_entry(struct entry *ent0)
{
	struct entry *ent;
	int i;

	if (ent0 == NULL)
		return;

	ent = ent0;
	while (ent != NULL)
	{
		if (ent->type == DERAR_TYPE_DIRECTORY)
			free_entry(ent->dir.subentry);	
		else
		{
			for (i = 0; i < ent->file.parts; i++)
				if (ent->file.part[i].fd != -1)
					close(ent->file.part[i].fd);

			free(ent->file.part);
		}

		ent0 = ent;
		ent = ent->next;

		free(ent0->name);
		free(ent0);
	}
}

struct entry *insert_entry(struct entry *parent, const char *name, enum derar_type type)
{
	struct entry *new = calloc(sizeof(struct entry), 1);

	new->name = strdup(name);
	new->type = type;

	if (parent != NULL)
	{
		new->next = parent->dir.subentry; /* Save old pointer as new next */
		parent->dir.subentry = new;       /* Repoint to new entry */
	}
	else
		new->next = NULL;

	return new;
}

void entry(struct entry *root, char *path, enum derar_type type, int fd, off_t off, off_t partsize)
{
	char *saveptr = NULL;
	char *str;

	struct entry *ent0 = NULL, *ent = root;

	for (str = strtok_r(path, "\\", &saveptr);
	     str != NULL;
	     str = strtok_r(NULL, "\\", &saveptr))
	{
		if (ent)
		{
			/* Look for str on the current level */
			for (ent0 = ent->dir.subentry; ent0; ent0 = ent0->next)
				if (strcmp(ent0->name, str) == 0)
					break;
		}

		if (ent0) /* str exists! */
			ent = ent0;
		else
			ent = insert_entry(ent, str, DERAR_TYPE_DIRECTORY);
	}

	ent->type = type;

	if (ent->type == DERAR_TYPE_FILE)
	{
		int p = ent->file.parts;

		ent->file.parts++;
		ent->file.part = realloc(ent->file.part, ent->file.parts * sizeof(struct part));

		ent->file.part[p].fd     = fd;
		ent->file.part[p].foff   = ent->file.size;
		ent->file.part[p].off    = (uint64_t)off;
		ent->file.part[p].size   = (uint64_t)partsize;

		ent->file.mode = 0444;
		ent->file.size += partsize;
#ifdef WIN32
		ent->file.part[p].handle = (HANDLE)_get_osfhandle(fd);
#endif
	}
}
