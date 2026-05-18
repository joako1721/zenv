#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "zenv/atomic.h"
#include "zenv/zenv.h"

static void fsync_parent(const char *path)
{
	char *copy = strdup(path);
	if (!copy) return;
	char *dir = dirname(copy);
	int dfd = open(dir, O_RDONLY | O_DIRECTORY);
	if (dfd >= 0) {
		(void)fsync(dfd);
		(void)close(dfd);
	}
	free(copy);
}

int zenv_atomic_write(const char *path, const void *buf, size_t len, mode_t mode)
{
	static const char suffix[] = ".tmp.XXXXXX";
	size_t plen = strlen(path);
	char  *tmp  = malloc(plen + sizeof suffix);
	if (!tmp) {
		zenv_set_error("atomic_write: out of memory");
		return -1;
	}
	memcpy(tmp, path, plen);
	memcpy(tmp + plen, suffix, sizeof suffix);

	int fd = mkstemp(tmp);
	if (fd < 0) {
		zenv_set_error("atomic_write: mkstemp(%s): %s", tmp, strerror(errno));
		free(tmp);
		return -1;
	}

	const unsigned char *p = buf;
	size_t remaining = len;
	while (remaining > 0) {
		ssize_t n = write(fd, p, remaining);
		if (n < 0) {
			if (errno == EINTR) continue;
			zenv_set_error("atomic_write: write(%s): %s",
			               tmp, strerror(errno));
			goto fail;
		}
		p         += (size_t)n;
		remaining -= (size_t)n;
	}

	if (fsync(fd) < 0) {
		zenv_set_error("atomic_write: fsync(%s): %s", tmp, strerror(errno));
		goto fail;
	}
	if (fchmod(fd, mode) < 0) {
		zenv_set_error("atomic_write: fchmod(%s): %s", tmp, strerror(errno));
		goto fail;
	}
	if (close(fd) < 0) {
		zenv_set_error("atomic_write: close(%s): %s", tmp, strerror(errno));
		fd = -1;
		goto fail;
	}
	fd = -1;

	if (rename(tmp, path) < 0) {
		zenv_set_error("atomic_write: rename(%s -> %s): %s",
		               tmp, path, strerror(errno));
		goto fail;
	}

	fsync_parent(path);
	free(tmp);
	return 0;

fail:
	if (fd >= 0) (void)close(fd);
	(void)unlink(tmp);
	free(tmp);
	return -1;
}
