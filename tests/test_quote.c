#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenv/quote.h"

static int failures = 0;

static void check(const char *desc, const char *in, const char *want)
{
	char *got = zenv_shell_quote(in);
	if (!got) {
		fprintf(stderr, "  FAIL %s: quote returned NULL\n", desc);
		failures++;
		return;
	}
	if (strcmp(got, want) != 0) {
		fprintf(stderr, "  FAIL %s\n    in:   %s\n    want: %s\n    got:  %s\n",
		        desc, in, want, got);
		failures++;
	} else {
		printf("  ok   %s\n", desc);
	}
	free(got);
}

int main(void)
{
	puts("== test_quote ==");

	check("empty",            "",        "''");
	check("plain",            "bar",     "'bar'");
	check("with single quote", "it's",   "'it'\\''s'");
	check("dollar and backtick", "$x`y`", "'$x`y`'");
	check("multiple quotes",  "a'b'c",   "'a'\\''b'\\''c'");
	check("only quote",       "'",       "''\\'''");
	check("newline embedded", "a\nb",    "'a\nb'");
	check("backslash",        "a\\b",    "'a\\b'");
	check("double quote",     "a\"b",    "'a\"b'");

	if (failures) {
		fprintf(stderr, "FAILED: %d\n", failures);
		return 1;
	}
	puts("all green");
	return 0;
}
