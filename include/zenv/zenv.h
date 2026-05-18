#ifndef ZENV_H
#define ZENV_H

#define ZENV_VERSION "0.1.0-dev"

const char *zenv_last_error(void);
void        zenv_set_error(const char *fmt, ...);
void        zenv_clear_error(void);

int zenv_cmd_shell_init(int argc, char **argv);
int zenv_cmd_var(int argc, char **argv);
int zenv_cmd_version(int argc, char **argv);

#endif
