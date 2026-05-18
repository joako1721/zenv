#include <stdio.h>

#include "zenv/zenv.h"

int zenv_cmd_version(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	printf("zenv %s\n", ZENV_VERSION);
	return 0;
}
