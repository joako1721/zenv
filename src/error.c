#include <stdarg.h>
#include <stdio.h>

#include "zenv/zenv.h"

static _Thread_local char err_buf[512];

const char *zenv_last_error(void)
{
	return err_buf;
}

void zenv_clear_error(void)
{
	err_buf[0] = '\0';
}

void zenv_set_error(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(err_buf, sizeof err_buf, fmt, ap);
	va_end(ap);
}
