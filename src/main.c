#include <stdio.h>
#include <string.h>

#include "zenv/zenv.h"

static int usage(FILE* out)
{
	fputs("usage: zenv <command> [args...]\n"
		  "\n"
		  "commands:\n"
		  "  shell-init <shell>     emit shell hook (eval in your rc)\n"
		  "  var set <name> <value> set persistent variable\n"
		  "  var get <name>         print variable value\n"
		  "  var unset <name>       remove persistent variable\n"
		  "  var ls                 list persistent variables\n"
		  "  version                print version\n"
		  "\n"
		  "mutating commands emit shell code on stdout; eval it.\n",
		out);
	return out == stderr ? 2 : 0;
}

int main(int argc, char** argv)
{
	if (argc < 2)
		return usage(stderr);

	const char* cmd = argv[1];
	int rc;

	if (!strcmp(cmd, "shell-init")) {
		rc = zenv_cmd_shell_init(argc - 2, argv + 2);
	} else if (!strcmp(cmd, "var")) {
		rc = zenv_cmd_var(argc - 2, argv + 2);
	} else if (!strcmp(cmd, "version") || !strcmp(cmd, "--version") || !strcmp(cmd, "-v")) {
		rc = zenv_cmd_version(argc - 2, argv + 2);
	} else if (!strcmp(cmd, "help") || !strcmp(cmd, "--help") || !strcmp(cmd, "-h")) {
		return usage(stdout);
	} else {
		fprintf(stderr, "zenv: unknown command: %s\n", cmd);
		return usage(stderr);
	}

	if (rc != 0) {
		const char* err = zenv_last_error();
		if (err && *err)
			fprintf(stderr, "zenv: %s\n", err);
		return 1;
	}
	return 0;
}
