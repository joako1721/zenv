#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "zenv/vars.h"
#include "zenv/zenv.h"

static int failures = 0;

static void fail_(const char *desc, const char *why)
{
	fprintf(stderr, "  FAIL %s: %s\n", desc, why);
	failures++;
}
static void ok_(const char *desc) { printf("  ok   %s\n", desc); }

static void test_set_get(void)
{
	zenv_vars_t *v = zenv_vars_new();
	if (zenv_vars_set(v, "IP", "10.10.14.129") != 0) {
		fail_("set+get basic", zenv_last_error());
	} else {
		const char *got = zenv_vars_get(v, "IP");
		if (!got || strcmp(got, "10.10.14.129") != 0)
			fail_("set+get basic", "value mismatch");
		else
			ok_("set+get basic");
	}
	zenv_vars_free(v);
}

static void test_overwrite(void)
{
	zenv_vars_t *v = zenv_vars_new();
	zenv_vars_set(v, "X", "a");
	zenv_vars_set(v, "X", "b");
	const char *got = zenv_vars_get(v, "X");
	if (!got || strcmp(got, "b") != 0)
		fail_("overwrite", "second value did not win");
	else if (zenv_vars_count(v) != 1)
		fail_("overwrite", "count != 1");
	else
		ok_("overwrite");
	zenv_vars_free(v);
}

static void test_unset(void)
{
	zenv_vars_t *v = zenv_vars_new();
	zenv_vars_set(v, "A", "1");
	zenv_vars_set(v, "B", "2");
	zenv_vars_set(v, "C", "3");
	if (zenv_vars_unset(v, "B") != 0) {
		fail_("unset", zenv_last_error());
		zenv_vars_free(v);
		return;
	}
	if (zenv_vars_get(v, "B"))           fail_("unset", "B still present");
	else if (!zenv_vars_get(v, "A"))     fail_("unset", "A gone");
	else if (!zenv_vars_get(v, "C"))     fail_("unset", "C gone");
	else if (zenv_vars_count(v) != 2)    fail_("unset", "count wrong");
	else                                 ok_("unset");
	zenv_vars_free(v);
}

static void test_unset_missing(void)
{
	zenv_vars_t *v = zenv_vars_new();
	if (zenv_vars_unset(v, "nope") == 0)
		fail_("unset missing", "expected failure");
	else
		ok_("unset missing");
	zenv_vars_free(v);
}

static void test_invalid_name(void)
{
	zenv_vars_t *v = zenv_vars_new();
	if (zenv_vars_set(v, "1bad", "x") == 0)
		fail_("invalid name (digit start)", "expected failure");
	else if (zenv_vars_set(v, "has space", "x") == 0)
		fail_("invalid name (space)", "expected failure");
	else if (zenv_vars_set(v, "", "x") == 0)
		fail_("invalid name (empty)", "expected failure");
	else if (zenv_vars_set(v, "_ok99", "x") != 0)
		fail_("valid name", zenv_last_error());
	else
		ok_("name validation");
	zenv_vars_free(v);
}

static void test_round_trip(const char *tmpdir)
{
	char path[256];
	snprintf(path, sizeof path, "%s/vars.toml", tmpdir);

	zenv_vars_t *v = zenv_vars_new();
	zenv_vars_set(v, "IP", "10.10.14.129");
	zenv_vars_set(v, "tip", "10.10.14.1");
	zenv_vars_set(v, "PAYLOAD", "a \"quoted\" string with\nnewline and \\ slash");
	if (zenv_vars_save(v, path) != 0) {
		fail_("round-trip save", zenv_last_error());
		zenv_vars_free(v);
		return;
	}
	zenv_vars_free(v);

	zenv_vars_t *r = zenv_vars_new();
	if (zenv_vars_load(r, path) != 0) {
		fail_("round-trip load", zenv_last_error());
		zenv_vars_free(r);
		return;
	}
	if (zenv_vars_count(r) != 3)
		fail_("round-trip", "count != 3");
	else if (strcmp(zenv_vars_get(r, "IP"), "10.10.14.129") != 0)
		fail_("round-trip IP", "mismatch");
	else if (strcmp(zenv_vars_get(r, "tip"), "10.10.14.1") != 0)
		fail_("round-trip tip", "mismatch");
	else if (strcmp(zenv_vars_get(r, "PAYLOAD"),
	                "a \"quoted\" string with\nnewline and \\ slash") != 0)
		fail_("round-trip PAYLOAD", "mismatch");
	else
		ok_("round-trip with escapes");
	zenv_vars_free(r);
}

static void test_missing_file_is_empty(const char *tmpdir)
{
	char path[256];
	snprintf(path, sizeof path, "%s/nope.toml", tmpdir);
	zenv_vars_t *v = zenv_vars_new();
	if (zenv_vars_load(v, path) != 0)
		fail_("missing file", zenv_last_error());
	else if (zenv_vars_count(v) != 0)
		fail_("missing file", "non-empty");
	else
		ok_("missing file loads as empty");
	zenv_vars_free(v);
}

static void test_mode_is_0600(const char *tmpdir)
{
	char path[256];
	snprintf(path, sizeof path, "%s/perm.toml", tmpdir);
	zenv_vars_t *v = zenv_vars_new();
	zenv_vars_set(v, "X", "1");
	zenv_vars_save(v, path);
	zenv_vars_free(v);

	struct stat st;
	stat(path, &st);
	if ((st.st_mode & 07777) != 0600)
		fail_("mode is 0600", "wrong mode");
	else
		ok_("mode is 0600");
}

int main(void)
{
	puts("== test_vars ==");

	char tmpdir[] = "/tmp/zenv-vars-XXXXXX";
	if (!mkdtemp(tmpdir)) {
		fprintf(stderr, "mkdtemp: %s\n", strerror(errno));
		return 1;
	}

	test_set_get();
	test_overwrite();
	test_unset();
	test_unset_missing();
	test_invalid_name();
	test_round_trip(tmpdir);
	test_missing_file_is_empty(tmpdir);
	test_mode_is_0600(tmpdir);

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
