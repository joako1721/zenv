#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "zenv/atomic.h"
#include "zenv/zenv.h"

static int failures = 0;

static void fail_(const char *desc, const char *why)
{
	fprintf(stderr, "  FAIL %s: %s\n", desc, why);
	failures++;
}

static void ok_(const char *desc) { printf("  ok   %s\n", desc); }

/* Read whole file into a malloc'd buffer; *len receives byte count.
 * Returns NULL on error. */
static char *slurp(const char *path, size_t *len)
{
	FILE *f = fopen(path, "rb");
	if (!f) return NULL;
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	char *buf = malloc((size_t)sz + 1);
	if (!buf) { fclose(f); return NULL; }
	size_t n = fread(buf, 1, (size_t)sz, f);
	fclose(f);
	buf[n] = '\0';
	*len = n;
	return buf;
}

/* Count files matching prefix in dir. Used to verify no .tmp leftovers. */
static int count_with_prefix(const char *dir, const char *prefix)
{
	DIR *d = opendir(dir);
	if (!d) return -1;
	int n = 0;
	struct dirent *e;
	size_t plen = strlen(prefix);
	while ((e = readdir(d))) {
		if (strncmp(e->d_name, prefix, plen) == 0) n++;
	}
	closedir(d);
	return n;
}

static void test_basic_write(const char *tmpdir)
{
	char path[256];
	snprintf(path, sizeof path, "%s/basic", tmpdir);

	const char *msg = "hello world";
	if (zenv_atomic_write(path, msg, strlen(msg), 0644) != 0) {
		fail_("basic write", zenv_last_error());
		return;
	}
	size_t n;
	char *got = slurp(path, &n);
	if (!got || n != strlen(msg) || memcmp(got, msg, n) != 0) {
		fail_("basic write", "contents mismatch");
		free(got);
		return;
	}
	free(got);
	ok_("basic write");
}

static void test_overwrite(const char *tmpdir)
{
	char path[256];
	snprintf(path, sizeof path, "%s/overwrite", tmpdir);

	(void)zenv_atomic_write(path, "first",  5, 0644);
	if (zenv_atomic_write(path, "second", 6, 0644) != 0) {
		fail_("overwrite", zenv_last_error());
		return;
	}
	size_t n;
	char *got = slurp(path, &n);
	if (n != 6 || memcmp(got, "second", 6) != 0) {
		fail_("overwrite", "second write did not replace first");
		free(got);
		return;
	}
	free(got);
	ok_("overwrite");
}

static void test_binary_safe(const char *tmpdir)
{
	char path[256];
	snprintf(path, sizeof path, "%s/binary", tmpdir);

	unsigned char data[256];
	for (int i = 0; i < 256; i++) data[i] = (unsigned char)i;

	if (zenv_atomic_write(path, data, sizeof data, 0644) != 0) {
		fail_("binary safe", zenv_last_error());
		return;
	}
	size_t n;
	char *got = slurp(path, &n);
	if (n != sizeof data || memcmp(got, data, n) != 0) {
		fail_("binary safe", "round-trip mismatch");
		free(got);
		return;
	}
	free(got);
	ok_("binary safe");
}

static void test_mode_bits(const char *tmpdir)
{
	char path[256];
	snprintf(path, sizeof path, "%s/secret", tmpdir);

	if (zenv_atomic_write(path, "x", 1, 0600) != 0) {
		fail_("mode bits", zenv_last_error());
		return;
	}
	struct stat st;
	if (stat(path, &st) != 0) {
		fail_("mode bits", "stat failed");
		return;
	}
	mode_t got_mode = st.st_mode & 07777;
	if (got_mode != 0600) {
		char msg[64];
		snprintf(msg, sizeof msg, "want 0600 got %04o", got_mode);
		fail_("mode bits", msg);
		return;
	}
	ok_("mode bits");
}

static void test_no_tmp_on_success(const char *tmpdir)
{
	char path[256];
	snprintf(path, sizeof path, "%s/cleanup", tmpdir);
	(void)zenv_atomic_write(path, "x", 1, 0644);
	if (count_with_prefix(tmpdir, "cleanup.tmp.") != 0) {
		fail_("no tmpfile on success", "tmpfile leaked");
		return;
	}
	ok_("no tmpfile on success");
}

static void test_no_tmp_on_failure(const char *tmpdir)
{
	char path[256];
	snprintf(path, sizeof path, "%s/nope/file", tmpdir);
	if (zenv_atomic_write(path, "x", 1, 0644) == 0) {
		fail_("error on bad path", "expected failure");
		return;
	}
	/* mkstemp itself fails in nonexistent dir, so no tmpfile is created
	 * anywhere. We mainly check the call returned -1 and set an error. */
	if (!zenv_last_error() || !*zenv_last_error()) {
		fail_("error on bad path", "no error message set");
		return;
	}
	ok_("error on bad path sets message");
}

static void test_empty_write(const char *tmpdir)
{
	char path[256];
	snprintf(path, sizeof path, "%s/empty", tmpdir);
	if (zenv_atomic_write(path, "", 0, 0644) != 0) {
		fail_("empty write", zenv_last_error());
		return;
	}
	struct stat st;
	stat(path, &st);
	if (st.st_size != 0) {
		fail_("empty write", "expected zero size");
		return;
	}
	ok_("empty write");
}

int main(void)
{
	puts("== test_atomic ==");

	char tmpdir[] = "/tmp/zenv-test-XXXXXX";
	if (!mkdtemp(tmpdir)) {
		fprintf(stderr, "mkdtemp failed: %s\n", strerror(errno));
		return 1;
	}

	test_basic_write(tmpdir);
	test_overwrite(tmpdir);
	test_binary_safe(tmpdir);
	test_mode_bits(tmpdir);
	test_no_tmp_on_success(tmpdir);
	test_no_tmp_on_failure(tmpdir);
	test_empty_write(tmpdir);

	/* best-effort cleanup */
	char cmd[300];
	snprintf(cmd, sizeof cmd, "rm -rf %s", tmpdir);
	(void)system(cmd);

	if (failures) {
		fprintf(stderr, "FAILED: %d\n", failures);
		return 1;
	}
	puts("all green");
	return 0;
}
