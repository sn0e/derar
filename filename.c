#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void path_format(char *str, const char *format, int n)
{
	size_t len;

	sprintf(str, format, n);

	if (n == -1)
	{
		len = strlen(str);

		str[len - 2] = 'a';
		str[len - 1] = 'r';
	}
}

char *path_parse(const char *path, int *n, int new)
{
	int num, i;
	size_t len = strlen(path);
	char *format;

	/* Test for invalid filename */
	if (len < 4 || path[len - 4] != '.' || tolower(path[len - 3]) != 'r' ||
	    !((tolower(path[len - 2]) == 'a' && tolower(path[len - 1]) == 'r') ||
		  (isdigit(path[len - 2]) && isdigit(path[len - 1]))))
		return NULL;

	/* Allocate the format string */
	if ((format = malloc(len + 3)) == NULL)
		return NULL;

	if (new)
		for (i = len - 10; i >= 0; i--)
			if (sscanf(path + i, ".part%d.rar", &num) == 1 && num >= 1)
			{
				sprintf(format + i, ".part%%0%lud.rar", (long unsigned)len - i - 9);
				memcpy(format, path, i + 5);
				*n = 1;

				return format;
			}

	sprintf(format + len - 2, "%%0%dd", 2);
	memcpy(format, path, len - 2);
	*n = -1;

	return format;
}

