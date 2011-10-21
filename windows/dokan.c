#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <windows.h>

#include <dokan.h>
#include <derar.h>

#include "ui.h"

static
int *drfs_volume_info(LPWSTR volume_name,
                      DWORD volume_name_size,
                      LPDWORD volume_serial_number,
                      LPDWORD maximum_component_length,
                      LPDWORD fs_flags,
                      LPWSTR fs_name,
                      DWORD fs_name_size,
                      PDOKAN_FILE_INFO file_info)
{
	struct derar *derar = (struct derar *)file_info->DokanOptions->GlobalContext;

	*volume_serial_number = 0;
	*maximum_component_length = 1024; /* fix this. */
	*fs_flags = FILE_READ_ONLY_VOLUME;
	MultiByteToWideChar(CP_UTF8, 0, derar_name(derar), -1, volume_name, volume_name_size);
	MultiByteToWideChar(CP_UTF8, 0, "deRAR", -1, fs_name, fs_name_size);

	return 0;
}

int drfs_getsize(PULONGLONG freeBytes,
                 PULONGLONG totalBytes,
                 PULONGLONG totalFreeBytes,
                 PDOKAN_FILE_INFO file_info)
{
	struct derar *derar = (struct dr_archive *)file_info->DokanOptions->GlobalContext;

	*freeBytes      = 0;
	*totalBytes     = derar_total_size(derar);
	*totalFreeBytes = derar_total_size(derar); /* to stop windows complaining. */

	return 0;
}

static
int drfs_getattr(LPCWSTR wpath,
                 LPBY_HANDLE_FILE_INFORMATION file_handle,
                 PDOKAN_FILE_INFO file_info)
{
	struct derar_handle *handle;
	uint64_t size;
	char path[MAX_PATH];

	WideCharToMultiByte(CP_UTF8, 0, wpath, -1, path, MAX_PATH, NULL, NULL);

	memset(file_handle, 0, sizeof(BY_HANDLE_FILE_INFORMATION));

	handle = (struct handle *)file_info->Context;

	if(file_info->IsDirectory)
	{
			file_handle->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_READONLY;
			file_handle->nFileSizeHigh  = 0;
			file_handle->nFileSizeLow   = 0;
			file_handle->nNumberOfLinks = 2;
			file_handle->nFileIndexHigh = 0;
			file_handle->nFileIndexLow  = 0;
	}
	else
	{
		size = derar_size(handle);

		file_handle->dwFileAttributes = FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_READONLY;
		file_handle->nFileSizeHigh  = (DWORD)(size >> 32);
		file_handle->nFileSizeLow   = (DWORD)(size);
		file_handle->nNumberOfLinks = 1;
		file_handle->nFileIndexHigh = 0;
		file_handle->nFileIndexLow  = 0;
	}

	return 0;
}

static void UnixTimeToFileTime(time_t t, LPFILETIME pft)
{
	long long ll;

	ll = Int32x32To64(t, 10000000) + 116444736000000000;
	pft->dwLowDateTime = (DWORD)ll;
	pft->dwHighDateTime = ll >> 32;
}

static
int drfs_readdir(LPCWSTR wpath,
                 PFillFindData fill,
                 PDOKAN_FILE_INFO file_info)
{
	struct derar_handle *dir;
	enum derar_type type;
	const char *name;
	time_t mtime;
	uint64_t size;

	WIN32_FIND_DATAW info;

	dir = (struct derar_handle *)file_info->Context;

	if(dir == NULL)
		return -ERROR_NOT_FOUND;
	else if (derar_type(dir) != DERAR_TYPE_DIRECTORY)
		return -ERROR_NOT_SUPPORTED;

	while (derar_readdir(dir, &name, &type, &mtime, &size))
	{
		if (type == DERAR_TYPE_DIRECTORY)
		{
			memset(&info, 0, sizeof(info));
			info.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_READONLY;
			UnixTimeToFileTime(mtime, &info.ftLastWriteTime);

			MultiByteToWideChar(CP_UTF8, 0, name, -1, info.cFileName, MAX_PATH);

			fill(&info, file_info);
		}
		else
		{
			memset(&info, 0, sizeof(info));
			info.dwFileAttributes = FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_READONLY;
			UnixTimeToFileTime(mtime, &info.ftLastWriteTime);
			info.nFileSizeHigh    = (DWORD)(size >> 32);
			info.nFileSizeLow     = (DWORD)(size);

			MultiByteToWideChar(CP_UTF8, 0, name, -1, info.cFileName, MAX_PATH);

			fill(&info, file_info);
		}
	}

	return 0;
}

static
int drfs_opendir(LPCWSTR wpath,
                 PDOKAN_FILE_INFO file_info)
{
	struct derar        *derar = file_info->DokanOptions->GlobalContext;
	struct derar_handle *dir;

	char path[MAX_PATH];

	if(!file_info->IsDirectory)
		return -ERROR_NOT_SUPPORTED;

	WideCharToMultiByte(CP_UTF8, 0, wpath, -1, path, MAX_PATH, NULL, NULL);

	if((dir = derar_open(derar, (const char *)path)) == NULL)
		return -ERROR_PATH_NOT_FOUND;

	if (derar_type(dir) != DERAR_TYPE_DIRECTORY)
	{
		derar_close(dir);
		return -ERROR_NO_MORE_ITEMS; /* STATUS_NOT_A_DIRECTORY 0xC0000103 */
	}

	file_info->Context = (ULONG64)dir;
	file_info->IsDirectory = TRUE;

	return 0;
}

static
int drfs_open(LPCWSTR wpath,
              DWORD desired_access,
              DWORD share_mode,
              DWORD creation_disposition,
              DWORD flags,
              PDOKAN_FILE_INFO file_info)
{
	struct derar *archive = file_info->DokanOptions->GlobalContext;
	struct derar_handle *file;
	char path[MAX_PATH];

	if(creation_disposition == CREATE_ALWAYS ||
	   creation_disposition == OPEN_ALWAYS)
//		return -ERROR_FILE_NOT_FOUND;
		return -ERROR_NOT_SUPPORTED;

	WideCharToMultiByte(CP_UTF8, 0, wpath, -1, path, MAX_PATH, NULL, NULL);

	if((file = derar_open(archive, (const char *)path)) != NULL)
	{
		file_info->IsDirectory = derar_type(file) == DERAR_TYPE_DIRECTORY;
		file_info->Context = (ULONG64)file;
	}
	else
		return -ERROR_FILE_NOT_FOUND;

	return 0;
}

static
int drfs_release(LPCWSTR path,
               PDOKAN_FILE_INFO file_info)
{
	if(file_info->Context != NULL)
		derar_close((struct derar_handle *)file_info->Context);

	return 0;
}

static
int drfs_read(LPCWSTR wpath,
              LPVOID buffer,
              DWORD size,
              LPDWORD numread,
              LONGLONG offset,
              PDOKAN_FILE_INFO file_info)
{
	struct derar_handle *file;

	file = (struct derar_handle *)file_info->Context;

	if (file_info->IsDirectory)
		return -ERROR_NOT_SUPPORTED;

	ssize_t ret = derar_read(file, buffer, size, offset);

	if(ret < 0)
	{
		*numread = 0;
		return -1;
	}

	*numread = ret;

	return 0;
}

void Mount(void *data)
{
	struct mount *mount = (struct mount *)data;
	int ret;

	DOKAN_OPERATIONS operations = {
		.CreateFile           = drfs_open,
		.OpenDirectory        = drfs_opendir,
		.FindFiles            = drfs_readdir,
		.CloseFile            = drfs_release,
		.ReadFile             = drfs_read,
		.GetFileInformation   = drfs_getattr,
		.GetVolumeInformation = drfs_volume_info,
		.GetDiskFreeSpace     = drfs_getsize
	};

	DOKAN_OPTIONS options = {
		.DriveLetter = mount->letter,
		.ThreadCount = 0,
		.GlobalContext = NULL
	};

	options.GlobalContext = (struct derar *)derar_initialize(mount->path);

	if (options.GlobalContext != NULL)
	{
		ret = DokanMain(&options, &operations);
		derar_deinitialize(options.GlobalContext);

		if (ret != DOKAN_SUCCESS)
		{
			LPCTSTR text;

			switch(ret)
			{
				case DOKAN_ERROR:
					text = "Mounting failed because of a general error.";
					break;
				case DOKAN_DRIVE_LETTER_ERROR:
					text = "Mounting failed because the drive letter you selected is bad.";
					break;
				case DOKAN_DRIVER_INSTALL_ERROR:
					text = "Mounting failed because the Dokan driver failed to install.";
					break;
				case DOKAN_START_ERROR:
					text = "Mounting failed because something went wrong with the Dokan driver.";
					break;
				case DOKAN_MOUNT_ERROR:
					text = "Mounting failed because the drive letter you selected could not be assigned.";
					break;
				default:
					text = "An unknown Dokan error has occured.";
					break;
			}

			MessageBox(NULL, text, "deRAR Mount Error", MB_OK | MB_ICONERROR);
		}
	}
	else
		MessageBox(NULL, "Could not mount archive.", "deRAR Mount Error", MB_OK | MB_ICONERROR);

	RemoveMenu(menu, mount->menu, MF_BYCOMMAND);
	mount->mounted = FALSE;
}

