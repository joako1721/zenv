#ifndef ZENV_PATHS_H
#define ZENV_PATHS_H

#include <sys/types.h>

/* "<XDG_STATE_HOME or ~/.local/state>/zenv/<leaf>". Creates the parent
 * dir if missing. Caller frees. NULL on error. */
char *zenv_state_path(const char *leaf);

int zenv_mkdir_p(const char *path, mode_t mode);

#endif
