#ifndef ZENV_VARS_H
#define ZENV_VARS_H

#include <stddef.h>

typedef struct zenv_vars zenv_vars_t;

zenv_vars_t *zenv_vars_new(void);
void         zenv_vars_free(zenv_vars_t *v);

/* Missing file is not an error; v is left empty. */
int zenv_vars_load(zenv_vars_t *v, const char *path);
int zenv_vars_save(const zenv_vars_t *v, const char *path);

/* name must match [A-Za-z_][A-Za-z0-9_]*. */
int         zenv_vars_set(zenv_vars_t *v, const char *name, const char *value);
const char *zenv_vars_get(const zenv_vars_t *v, const char *name);
int         zenv_vars_unset(zenv_vars_t *v, const char *name);

size_t zenv_vars_count(const zenv_vars_t *v);
void   zenv_vars_at(const zenv_vars_t *v, size_t i,
                    const char **name, const char **value);

int zenv_vars_name_valid(const char *name);

#endif
