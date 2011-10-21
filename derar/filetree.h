#ifndef __FILETREE_H
#define __FILETREE_H

#include <fcntl.h>       /* off_t */
#include "derar.h"       /* enum derar_type */
#include "derar_types.h" /* struct entry */

void free_entry(struct entry *);
void entry(struct entry *, char *, enum derar_type, int, off_t, off_t);
struct entry *insert_entry(struct entry *, const char *, enum derar_type);

#endif

