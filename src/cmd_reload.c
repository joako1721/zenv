#include <stdio.h>
#include <stdlib.h>

#include "zenv/emit.h"
#include "zenv/paths.h"
#include "zenv/vars.h"
#include "zenv/zenv.h"

int zenv_cmd_reload(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	char *path = zenv_state_path("vars.toml");
	if (!path) return -1;

	zenv_vars_t *v = zenv_vars_new();
	if (!v) { free(path); return -1; }

	int rc = zenv_vars_load(v, path);
	if (rc == 0)
		rc = zenv_emit_state(stdout, v, getenv("_ZENV_LOADED"));

	zenv_vars_free(v);
	free(path);
	return rc;
}
