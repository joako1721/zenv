#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenv/paths.h"
#include "zenv/quote.h"
#include "zenv/vars.h"
#include "zenv/zenv.h"

static zenv_vars_t *load_vars(char **out_path)
{
	char *path = zenv_state_path("vars.toml");
	if (!path) return NULL;
	zenv_vars_t *v = zenv_vars_new();
	if (!v) { free(path); return NULL; }
	if (zenv_vars_load(v, path) != 0) {
		zenv_vars_free(v);
		free(path);
		return NULL;
	}
	*out_path = path;
	return v;
}

static int cmd_set(int argc, char **argv)
{
	if (argc < 2) {
		zenv_set_error("var set: usage: var set <name> <value>");
		return -1;
	}
	const char *name  = argv[0];
	const char *value = argv[1];

	char *path;
	zenv_vars_t *v = load_vars(&path);
	if (!v) return -1;

	int rc = zenv_vars_set(v, name, value);
	if (rc == 0) rc = zenv_vars_save(v, path);
	if (rc == 0) {
		char *q = zenv_shell_quote(value);
		if (!q) rc = -1;
		else {
			printf("export %s=%s\n", name, q);
			free(q);
		}
	}
	zenv_vars_free(v);
	free(path);
	return rc;
}

static int cmd_get(int argc, char **argv)
{
	if (argc < 1) {
		zenv_set_error("var get: usage: var get <name>");
		return -1;
	}
	char *path;
	zenv_vars_t *v = load_vars(&path);
	if (!v) return -1;
	const char *val = zenv_vars_get(v, argv[0]);
	int rc = 0;
	if (val) {
		printf("%s\n", val);
	} else {
		zenv_set_error("variable not set: %s", argv[0]);
		rc = -1;
	}
	zenv_vars_free(v);
	free(path);
	return rc;
}

static int cmd_unset(int argc, char **argv)
{
	if (argc < 1) {
		zenv_set_error("var unset: usage: var unset <name>");
		return -1;
	}
	char *path;
	zenv_vars_t *v = load_vars(&path);
	if (!v) return -1;

	int rc = zenv_vars_unset(v, argv[0]);
	if (rc == 0) rc = zenv_vars_save(v, path);
	if (rc == 0) printf("unset %s\n", argv[0]);

	zenv_vars_free(v);
	free(path);
	return rc;
}

static int cmd_ls(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	char *path;
	zenv_vars_t *v = load_vars(&path);
	if (!v) return -1;
	size_t n = zenv_vars_count(v);
	for (size_t i = 0; i < n; i++) {
		const char *name, *value;
		zenv_vars_at(v, i, &name, &value);
		printf("%s=%s\n", name, value);
	}
	zenv_vars_free(v);
	free(path);
	return 0;
}

int zenv_cmd_var(int argc, char **argv)
{
	if (argc < 1) {
		zenv_set_error("var: missing subcommand (set|get|unset|ls)");
		return -1;
	}
	const char *sub = argv[0];
	if (!strcmp(sub, "set"))   return cmd_set(argc - 1, argv + 1);
	if (!strcmp(sub, "get"))   return cmd_get(argc - 1, argv + 1);
	if (!strcmp(sub, "unset")) return cmd_unset(argc - 1, argv + 1);
	if (!strcmp(sub, "ls"))    return cmd_ls(argc - 1, argv + 1);

	zenv_set_error("var: unknown subcommand '%s'", sub);
	return -1;
}
