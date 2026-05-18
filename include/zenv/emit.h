#ifndef ZENV_EMIT_H
#define ZENV_EMIT_H

#include <stdio.h>

#include "zenv/vars.h"

/* Emit shell code to `out`:
 *   - unset for every name in loaded_csv not in current
 *   - export NAME=<quoted value> for every entry in current
 *   - export _ZENV_LOADED='space-separated names'
 * loaded_csv may be NULL (fresh shell, no diff). */
int zenv_emit_state(FILE *out, const zenv_vars_t *current,
                    const char *loaded_csv);

#endif
