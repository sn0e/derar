#ifndef _DERAR_H
#define _DERAR_H

#define __need_size_t
#define __need_ssize_t
#include <stddef.h> /* size_t, ssize_t */
#include <time.h>   /* time_t */
#include <stdint.h> /* uint64_t */

enum derar_type { DERAR_TYPE_DIRECTORY, DERAR_TYPE_FILE, DERAR_TYPE_INVALID };

struct derar;
struct derar_handle;

struct derar        *derar_initialize  (const char *);
struct derar_handle *derar_open        (struct derar *, const char *);
void                 derar_deinitialize(struct derar *);
const char          *derar_name        (struct derar *);
uint64_t             derar_total_size  (struct derar *);

int                  derar_readdir     (struct derar_handle *, const char **, enum derar_type *, time_t *, uint64_t *);
ssize_t              derar_read        (struct derar_handle *, void *, size_t, uint64_t);
enum derar_type      derar_type        (struct derar_handle *);
void                 derar_close       (struct derar_handle *);
uint64_t             derar_size        (struct derar_handle *);

#endif
