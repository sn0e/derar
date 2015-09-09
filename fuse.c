#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#define FUSE_USE_VERSION 26
#include <fuse/fuse.h>

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
	ssize_t ret;

	(void)path;

	ret = derar_read(handle, buf, size, (uint64_t)offset);

	if(ret < 0)
	{
		perror("derar_read");
		return -errno;
	}
	else
		return (int)ret;
}

static char *path = NULL;
static char *mountpoint = NULL;

static struct fuse_operations ops = {
	.getattr = drfs_getattr,
	.readdir = drfs_readdir,
	.open    = drfs_open,
	.release = drfs_release,
	.read    = drfs_read
};

enum {
	KEY_HELP
};

static struct fuse_opt drfs_opts[] = {
		FUSE_OPT_KEY("-h", KEY_HELP),
		FUSE_OPT_KEY("--help", KEY_HELP),
		FUSE_OPT_END
};

static int opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs)
{
	(void) data;
	if (key == FUSE_OPT_KEY_NONOPT) {
        if (path == NULL) {
            path = strdup(arg);
            return 0;
        } else if (mountpoint == NULL) {
            mountpoint = strdup(arg);
        }
	} else if (key == KEY_HELP) {
		fprintf(stderr, "Usage: %s [FUSE options] [--] <rarfile> <mount-point>\n\n", outargs->argv[0]);
		fuse_opt_add_arg(outargs, "-ho");
		fuse_main(outargs->argc, outargs->argv, &ops, NULL);
		exit(0);
	}

	return 1;
}

int main(int argc, char **argv)
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	fuse_opt_parse(&args, NULL, drfs_opts, opt_proc);

	if(!path)
	{
		fprintf(stderr, "rar archive required\n");
		exit(1);
	}

    if (!mountpoint)
    {
        fprintf(stderr, "mountpoint required\n");
        exit(1);
    }

    fprintf(stderr, "archive: %s\n", path);
    fprintf(stderr, "mountpoint: %s\n", mountpoint);

    if ((path = realpath(path, NULL)) == NULL)
	{
		perror("realpath");
		return -1;
	}

	derar = derar_initialize(path);

	free(path);

	if(derar == NULL)
	{
		return -1;
	}

	int ret = fuse_main(args.argc, args.argv, &ops, NULL);

	derar_deinitialize(derar);

	return ret;
}
