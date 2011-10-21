#ifndef __DERAR_INTERNAL_H
#define __DERAR_INTERNAL_H

#include <stdint.h> /* int64_t */
#include <time.h>   /* time_t */

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define WINVER 0x501
#include <windows.h>
#endif

struct entry;

struct derar
{
	char *name;
	int compressed;     /* Contains one ore more compressed files */
	uint64_t size;      /* The sum of all filesizes */

	struct entry *root; /* The root directory */
};

struct part
{
	int fd;        /* File descriptor of RAR-file */
#ifdef WIN32
	HANDLE handle;
#endif
	int number;

	time_t ftime;
	uint64_t off;  /* Data offset in the RAR-file */
	uint64_t foff; /* Starting offset in the file */
	uint64_t size; /* Size of chunk */
};

struct derar_handle
{
	struct entry *entry;

	union
	{
		struct
		{
			struct entry *current;

		} dir;

		struct
		{
			struct part *part;
			uint64_t off;

		} file;
	};
};

struct entry
{
	char *name;
	enum derar_type type;

	union
	{
		struct
		{
			struct entry *subentry;

		} dir;

		struct
		{
			mode_t mode;
			time_t time;
			uint64_t  size;

			struct part *part;
			int parts;

		} file;
	};

	struct entry *next;
};

#endif

