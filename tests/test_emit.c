#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenv/emit.h"
#include "zenv/vars.h"
#include "zenv/zenv.h"

static int failures = 0;

static void fail_(const char *desc, const char *why)
{
	fprintf(stderr, "  FAIL %s: %s\n", desc, why);
	failures++;
}
static void ok_(const char *desc) { printf("  ok   %s\n", desc); }

static char *capture(const zenv_vars_t *v, const char *loaded)
{
	FILE *fp = tmpfile();
	if (!fp) return NULL;
	if (zenv_emit_state(fp, v, loaded) != 0) {
		fclose(fp);
		return NULL;
	}
	long sz = ftell(fp);
	rewind(fp);
	char *buf = malloc((size_t)sz + 1);
	fread(buf, 1, (size_t)sz, fp);
	buf[sz] = '\0';
	fclose(fp);
	return buf;
}

static int contains(const char *hay, const char *needle)
{
	return strstr(hay, needle) != NULL;
}

static void test_fresh(void)
{
	zenv_vars_t *v = zenv_vars_new();
	zenv_vars_set(v, "IP",  "10.10.14.129");
	zenv_vars_set(v, "tip", "10.10.14.1");
	char *out = capture(v, NULL);
	if (!out)                                         fail_("fresh", "capture failed");
	else if (!contains(out, "export IP='10.10.14.129'"))   fail_("fresh", "missing IP export");
	else if (!contains(out, "export tip='10.10.14.1'"))    fail_("fresh", "missing tip export");
	else if (!contains(out, "export _ZENV_LOADED='IP tip'")) fail_("fresh", "missing _ZENV_LOADED");
	else if (contains(out, "unset"))                  fail_("fresh", "unexpected unset");
	else                                              ok_("fresh emit");
	free(out);
	zenv_vars_free(v);
}

static void test_diff_unsets_removed(void)
{
	zenv_vars_t *v = zenv_vars_new();
	zenv_vars_set(v, "A", "1");
	char *out = capture(v, "A B C");
	if (!out)                              fail_("diff", "capture failed");
	else if (!contains(out, "unset B"))    fail_("diff", "missing unset B");
	else if (!contains(out, "unset C"))    fail_("diff", "missing unset C");
	else if (contains(out, "unset A"))     fail_("diff", "should keep A");
	else if (!contains(out, "export A='1'")) fail_("diff", "missing A export");
	else if (!contains(out, "export _ZENV_LOADED='A'")) fail_("diff", "wrong _ZENV_LOADED");
	else                                   ok_("diff: unsets removed names");
	free(out);
	zenv_vars_free(v);
}

static void test_empty(void)
{
	zenv_vars_t *v = zenv_vars_new();
	char *out = capture(v, "OLD");
	if (!out)                                fail_("empty", "capture failed");
	else if (!contains(out, "unset OLD"))    fail_("empty", "missing unset");
	else if (!contains(out, "export _ZENV_LOADED=''")) fail_("empty", "wrong loaded");
	else                                     ok_("empty current state");
	free(out);
	zenv_vars_free(v);
}

int main(void)
{
	puts("== test_emit ==");
	test_fresh();
	test_diff_unsets_removed();
	test_empty();
	if (failures) { fprintf(stderr, "FAILED: %d\n", failures); return 1; }
	puts("all green");
	return 0;
}
