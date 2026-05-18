#include <stdlib.h>

#include "zenv/quote.h"
#include "zenv/zenv.h"

char *zenv_shell_quote(const char *value)
{
	size_t out_len = 2 + 1;
	for (const char *p = value; *p; p++)
		out_len += (*p == '\'') ? 4 : 1;

	char *out = malloc(out_len);
	if (!out) {
		zenv_set_error("shell_quote: out of memory");
		return NULL;
	}

	char *w = out;
	*w++ = '\'';
	for (const char *p = value; *p; p++) {
		if (*p == '\'') {
			*w++ = '\'';
			*w++ = '\\';
			*w++ = '\'';
			*w++ = '\'';
		} else {
			*w++ = *p;
		}
	}
	*w++ = '\'';
	*w   = '\0';

	return out;
}
