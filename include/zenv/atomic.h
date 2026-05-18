#ifndef ZENV_ATOMIC_H
#define ZENV_ATOMIC_H

#include <stddef.h>
#include <sys/types.h>

/* Write buf atomically to path: tmpfile + fsync + rename. Mode is applied
 * before publish. Returns 0 / -1 (sets zenv error). */
int zenv_atomic_write(const char *path, const void *buf, size_t len, mode_t mode);

#endif
