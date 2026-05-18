#include <stdlib.h>
#include <string.h>

#include "zenv/emit.h"
#include "zenv/quote.h"
#include "zenv/vars.h"
#include "zenv/zenv.h"

int zenv_emit_state(FILE *out, const zenv_vars_t *current, const char *loaded_csv)
{
	if (loaded_csv && *loaded_csv) {
		char *copy = strdup(loaded_csv);
		if (!copy) {
			zenv_set_error("emit: out of memory");
			return -1;
		}
		char *saveptr = NULL;
		for (char *tok = strtok_r(copy, " ", &saveptr); tok;
		     tok = strtok_r(NULL, " ", &saveptr)) {
			if (!zenv_vars_get(current, tok))
				fprintf(out, "unset %s\n", tok);
		}
		free(copy);
	}

	size_t n = zenv_vars_count(current);
	for (size_t i = 0; i < n; i++) {
		const char *name, *value;
		zenv_vars_at(current, i, &name, &value);
		char *q = zenv_shell_quote(value);
		if (!q) return -1;
		fprintf(out, "export %s=%s\n", name, q);
		free(q);
	}

	fputs("export _ZENV_LOADED='", out);
	for (size_t i = 0; i < n; i++) {
		const char *name, *value;
		zenv_vars_at(current, i, &name, &value);
		if (i > 0) fputc(' ', out);
		fputs(name, out);
	}
	fputs("'\n", out);

	return 0;
}
