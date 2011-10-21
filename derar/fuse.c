#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#define FUSE_USE_VERSION 26
#include <fuse/fuse.h>
#include <fuse/fuse_opt.h>

#include "derar.h"

static struct derar *derar = NULL;

static int drfs_getattr(const char *path, struct stat *stbuf)
{
	struct derar_handle *handle;

	memset(stbuf, 0, sizeof(struct stat));

	if (path[0] == '/' && path[1] == '\0')
	{
		stbuf->st_mode = S_IFDIR | 0555;
		stbuf->st_nlink = 2;
	}
	else
	{
		if ((handle = derar_open(derar, path + 1)) == NULL)
			return -errno;

		if (derar_type(handle) == DERAR_TYPE_FILE)
		{
			stbuf->st_nlink = 1;
			stbuf->st_size  = derar_size(handle);
			stbuf->st_mtime = time(NULL);

			stbuf->st_mode = 0444 | S_IFREG;
		}
		else
		{
			stbuf->st_nlink = 2;
			stbuf->st_size  = 0;
			stbuf->st_mtime = 0;

			stbuf->st_mode = 0555 | S_IFDIR;
		}

		derar_close(handle);
	}

	return 0;
}

static int drfs_readdir(const char *path, void *buf, fuse_fill_dir_t fill, off_t offset, struct fuse_file_info *fi)
{
	struct stat stbuf;
	struct derar_handle *handle;
	enum derar_type type;
	const char *name;
	time_t mtime;
	uint64_t size;

	(void)offset;
	(void)fi;

	fill(buf, ".", NULL, 0);
	fill(buf, "..", NULL, 0);

	memset(&stbuf, 0, sizeof(stbuf));
	stbuf.st_nlink = 1;

	handle = derar_open(derar, path + 1);

	if (handle == NULL)
		return -errno;

	while (derar_readdir(handle, &name, &type, &mtime, &size) > 0)
	{
		if (type == DERAR_TYPE_FILE)
		{
			stbuf.st_size  = size;
			stbuf.st_mtime = mtime;
			stbuf.st_mode = 0444 | S_IFREG;
		}
		else
		{
			stbuf.st_size  = 0;
			stbuf.st_mtime = 0;
			stbuf.st_mode = 0555 | S_IFDIR;
		}

		fill(buf, name, &stbuf, 0);
	}

	derar_close(handle);

	return 0;
}

static int drfs_open(const char *path, struct fuse_file_info *fi)
{
	struct derar_handle *handle;

	if (fi->flags & O_NONBLOCK || fi->flags & O_NDELAY)
		fprintf(stderr, "warning: ignoring nonblocking open flag\n");

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	if ((handle = derar_open(derar, path + 1)) == NULL)
		return -errno;

	fi->fh = (uint64_t)handle;

	return 0;
}

static int drfs_release(const char *path, struct fuse_file_info *fi)
{
	struct derar_handle *handle = (struct derar_handle *)fi->fh;

	(void)path;

	derar_close(handle);

	return 0;
}

static int drfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	struct derar_handle *handle = (struct derar_handle *)fi->fh;
	int ret;

	(void)path;

	ret = derar_read(handle, buf, size, offset);

	if(ret < 0)
	{
		perror("derar_read");
		return -errno;
	}
	else
		return ret;
}

int main(int argc, char **argv)
{
	int ret, i;
	char *path = NULL;
	struct fuse_operations operations;
	struct fuse_args args = FUSE_ARGS_INIT(0, NULL);

	if(argc < 2)
	{
		fprintf(stderr, "usage: %s <archive> [fuse options]\n", argv[0]);
		return -1;
	}

	memset(&operations, 0, sizeof(operations));

	operations.getattr = drfs_getattr;
	operations.readdir = drfs_readdir;
	operations.open    = drfs_open;
	operations.release = drfs_release;
	operations.read    = drfs_read;

	for (i = 0; i < argc; i++)
		if (i == 1)
			path = argv[1];
		else
			fuse_opt_add_arg(&args, argv[i]);

	if ((path = realpath(path, NULL)) == NULL)
	{
		perror("realpath");
		return -1;
	}

	derar = derar_initialize(path);

	free(path);

	if(derar == NULL)
	{
		perror("derar_initialize");
		return -1;
	}

	ret = fuse_main(args.argc, args.argv, &operations, NULL);

	derar_deinitialize(derar);

	return ret;
}

