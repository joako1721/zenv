#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zenv/paths.h"
#include "zenv/quote.h"
#include "zenv/vars.h"
#include "zenv/zenv.h"

static int emit_export(const char *name, const char *value)
{
	char *q = zenv_shell_quote(value);
	if (!q) return -1;
	printf("export %s=%s\n", name, q);
	free(q);
	return 0;
}

static int emit_persistent_vars(void)
{
	char *path = zenv_state_path("vars.toml");
	if (!path) return -1;
	zenv_vars_t *v = zenv_vars_new();
	if (!v) { free(path); return -1; }
	int rc = zenv_vars_load(v, path);
	if (rc == 0) {
		size_t n = zenv_vars_count(v);
		for (size_t i = 0; i < n && rc == 0; i++) {
			const char *name, *value;
			zenv_vars_at(v, i, &name, &value);
			rc = emit_export(name, value);
		}
	}
	zenv_vars_free(v);
	free(path);
	return rc;
}

static int emit_zsh(void)
{
	fputs(
	    "# --- zenv shell-init (zsh) ---\n"
	    "zenv() {\n"
	    "  local _mut=0\n"
	    "  case \"$1\" in\n"
	    "    var)  case \"$2\" in set|unset)            _mut=1 ;; esac ;;\n"
	    "    mode) case \"$2\" in off|'')               _mut=1\n"
	    "                          ;;\n"
	    "                       ls)                    _mut=0 ;;\n"
	    "                       *)                     _mut=1 ;;\n"
	    "          esac ;;\n"
	    "    cwd)  case \"$2\" in save|scope|restore)   _mut=1 ;; esac ;;\n"
	    "    reload)                                   _mut=1 ;;\n"
	    "  esac\n"
	    "  if [ \"$_mut\" = 1 ]; then\n"
	    "    local _out\n"
	    "    _out=\"$(command zenv \"$@\")\" || return $?\n"
	    "    [ -n \"$_out\" ] && eval \"$_out\"\n"
	    "  else\n"
	    "    command zenv \"$@\"\n"
	    "  fi\n"
	    "}\n",
	    stdout);

	if (emit_persistent_vars() != 0) return -1;

	fputs("# --- end zenv shell-init ---\n", stdout);
	return 0;
}

int zenv_cmd_shell_init(int argc, char **argv)
{
	if (argc < 1) {
		zenv_set_error("shell-init: missing shell name (try: zsh)");
		return -1;
	}
	if (!strcmp(argv[0], "zsh")) return emit_zsh();

	zenv_set_error("shell-init: unsupported shell '%s'", argv[0]);
	return -1;
}
