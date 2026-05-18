#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "zenv/paths.h"
#include "zenv/zenv.h"

int zenv_mkdir_p(const char *path, mode_t mode)
{
	char *copy = strdup(path);
	if (!copy) {
		zenv_set_error("mkdir_p: out of memory");
		return -1;
	}

	for (char *p = copy + 1; *p; p++) {
		if (*p != '/') continue;
		*p = '\0';
		if (mkdir(copy, mode) != 0 && errno != EEXIST) {
			zenv_set_error("mkdir(%s): %s", copy, strerror(errno));
			free(copy);
			return -1;
		}
		*p = '/';
	}
	if (mkdir(copy, mode) != 0 && errno != EEXIST) {
		zenv_set_error("mkdir(%s): %s", copy, strerror(errno));
		free(copy);
		return -1;
	}
	free(copy);
	return 0;
}

char *zenv_state_path(const char *leaf)
{
	const char *base = getenv("XDG_STATE_HOME");
	char *fallback = NULL;
	if (!base || !*base) {
		const char *home = getenv("HOME");
		if (!home || !*home) {
			zenv_set_error("state_path: neither XDG_STATE_HOME nor HOME set");
			return NULL;
		}
		size_t hlen = strlen(home);
		fallback = malloc(hlen + sizeof("/.local/state"));
		if (!fallback) {
			zenv_set_error("state_path: out of memory");
			return NULL;
		}
		memcpy(fallback, home, hlen);
		memcpy(fallback + hlen, "/.local/state", sizeof("/.local/state"));
		base = fallback;
	}

	size_t need = strlen(base) + strlen("/zenv/") + strlen(leaf) + 1;
	char *out  = malloc(need);
	if (!out) {
		zenv_set_error("state_path: out of memory");
		free(fallback);
		return NULL;
	}
	snprintf(out, need, "%s/zenv/%s", base, leaf);
	free(fallback);

	/* Ensure the parent directory exists. */
	char *slash = strrchr(out, '/');
	if (slash && slash != out) {
		*slash = '\0';
		if (zenv_mkdir_p(out, 0700) != 0) {
			free(out);
			return NULL;
		}
		*slash = '/';
	}
	return out;
}
