#ifndef ZENV_QUOTE_H
#define ZENV_QUOTE_H

/* Single-quote wrap `value` for safe shell eval. Caller frees.
 * NULL on OOM (sets zenv error). */
char *zenv_shell_quote(const char *value);

#endif
