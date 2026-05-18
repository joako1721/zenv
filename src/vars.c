#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "toml.h"

#include "zenv/atomic.h"
#include "zenv/vars.h"
#include "zenv/zenv.h"

struct kv {
	char *name;
	char *value;
};

struct zenv_vars {
	struct kv *items;
	size_t     len;
	size_t     cap;
};

zenv_vars_t *zenv_vars_new(void)
{
	zenv_vars_t *v = calloc(1, sizeof *v);
	if (!v) {
		zenv_set_error("vars_new: out of memory");
		return NULL;
	}
	return v;
}

void zenv_vars_free(zenv_vars_t *v)
{
	if (!v) return;
	for (size_t i = 0; i < v->len; i++) {
		free(v->items[i].name);
		free(v->items[i].value);
	}
	free(v->items);
	free(v);
}

int zenv_vars_name_valid(const char *name)
{
	if (!name || !*name) return 0;
	if (!(isalpha((unsigned char)*name) || *name == '_')) return 0;
	for (const char *p = name + 1; *p; p++)
		if (!(isalnum((unsigned char)*p) || *p == '_')) return 0;
	return 1;
}

static struct kv *find(const zenv_vars_t *v, const char *name)
{
	for (size_t i = 0; i < v->len; i++)
		if (strcmp(v->items[i].name, name) == 0) return &v->items[i];
	return NULL;
}

static int grow(zenv_vars_t *v)
{
	if (v->len < v->cap) return 0;
	size_t cap = v->cap ? v->cap * 2 : 8;
	struct kv *p = realloc(v->items, cap * sizeof *p);
	if (!p) {
		zenv_set_error("vars: out of memory");
		return -1;
	}
	v->items = p;
	v->cap   = cap;
	return 0;
}

int zenv_vars_set(zenv_vars_t *v, const char *name, const char *value)
{
	if (!zenv_vars_name_valid(name)) {
		zenv_set_error("invalid variable name: %s "
		               "(must match [A-Za-z_][A-Za-z0-9_]*)", name);
		return -1;
	}
	struct kv *existing = find(v, name);
	if (existing) {
		char *new_val = strdup(value);
		if (!new_val) {
			zenv_set_error("vars_set: out of memory");
			return -1;
		}
		free(existing->value);
		existing->value = new_val;
		return 0;
	}
	if (grow(v) != 0) return -1;
	char *n = strdup(name);
	char *w = strdup(value);
	if (!n || !w) {
		free(n);
		free(w);
		zenv_set_error("vars_set: out of memory");
		return -1;
	}
	v->items[v->len].name  = n;
	v->items[v->len].value = w;
	v->len++;
	return 0;
}

const char *zenv_vars_get(const zenv_vars_t *v, const char *name)
{
	struct kv *kv = find(v, name);
	return kv ? kv->value : NULL;
}

int zenv_vars_unset(zenv_vars_t *v, const char *name)
{
	for (size_t i = 0; i < v->len; i++) {
		if (strcmp(v->items[i].name, name) != 0) continue;
		free(v->items[i].name);
		free(v->items[i].value);
		memmove(&v->items[i], &v->items[i + 1],
		        (v->len - i - 1) * sizeof v->items[0]);
		v->len--;
		return 0;
	}
	zenv_set_error("variable not set: %s", name);
	return -1;
}

size_t zenv_vars_count(const zenv_vars_t *v) { return v->len; }

void zenv_vars_at(const zenv_vars_t *v, size_t i,
                  const char **name, const char **value)
{
	*name  = v->items[i].name;
	*value = v->items[i].value;
}

int zenv_vars_load(zenv_vars_t *v, const char *path)
{
	FILE *fp = fopen(path, "r");
	if (!fp) {
		if (errno == ENOENT) return 0;
		zenv_set_error("vars_load: open(%s): %s", path, strerror(errno));
		return -1;
	}
	char errbuf[256];
	toml_table_t *t = toml_parse_file(fp, errbuf, sizeof errbuf);
	fclose(fp);
	if (!t) {
		zenv_set_error("vars_load: parse error in %s: %s", path, errbuf);
		return -1;
	}

	int n = toml_table_nkval(t);
	for (int i = 0; i < n; i++) {
		const char *key = toml_key_in(t, i);
		toml_datum_t d  = toml_string_in(t, key);
		if (!d.ok) {
			zenv_set_error("vars_load: value for %s is not a string", key);
			toml_free(t);
			return -1;
		}
		int rc = zenv_vars_set(v, key, d.u.s);
		free(d.u.s);
		if (rc != 0) {
			toml_free(t);
			return -1;
		}
	}
	toml_free(t);
	return 0;
}

static int append_escaped(char **buf, size_t *len, size_t *cap, const char *s)
{
	for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
		char tmp[8];
		const char *seq = tmp;
		size_t seq_len;
		switch (*p) {
			case '"':  seq = "\\\"";  seq_len = 2; break;
			case '\\': seq = "\\\\";  seq_len = 2; break;
			case '\b': seq = "\\b";   seq_len = 2; break;
			case '\t': seq = "\\t";   seq_len = 2; break;
			case '\n': seq = "\\n";   seq_len = 2; break;
			case '\f': seq = "\\f";   seq_len = 2; break;
			case '\r': seq = "\\r";   seq_len = 2; break;
			default:
				if (*p < 0x20 || *p == 0x7f) {
					snprintf(tmp, sizeof tmp, "\\u%04X", *p);
					seq_len = strlen(tmp);
				} else {
					tmp[0] = (char)*p;
					seq_len = 1;
				}
				break;
		}
		if (*len + seq_len + 1 > *cap) {
			size_t ncap = *cap ? *cap * 2 : 256;
			while (ncap < *len + seq_len + 1) ncap *= 2;
			char *nbuf = realloc(*buf, ncap);
			if (!nbuf) {
				zenv_set_error("vars_save: out of memory");
				return -1;
			}
			*buf = nbuf;
			*cap = ncap;
		}
		memcpy(*buf + *len, seq, seq_len);
		*len += seq_len;
	}
	return 0;
}

static int append_raw(char **buf, size_t *len, size_t *cap, const char *s)
{
	size_t slen = strlen(s);
	if (*len + slen + 1 > *cap) {
		size_t ncap = *cap ? *cap * 2 : 256;
		while (ncap < *len + slen + 1) ncap *= 2;
		char *nbuf = realloc(*buf, ncap);
		if (!nbuf) {
			zenv_set_error("vars_save: out of memory");
			return -1;
		}
		*buf = nbuf;
		*cap = ncap;
	}
	memcpy(*buf + *len, s, slen);
	*len += slen;
	return 0;
}

int zenv_vars_save(const zenv_vars_t *v, const char *path)
{
	char  *buf = NULL;
	size_t len = 0, cap = 0;

	for (size_t i = 0; i < v->len; i++) {
		if (append_raw(&buf, &len, &cap, v->items[i].name) != 0) goto fail;
		if (append_raw(&buf, &len, &cap, " = \"")           != 0) goto fail;
		if (append_escaped(&buf, &len, &cap, v->items[i].value) != 0) goto fail;
		if (append_raw(&buf, &len, &cap, "\"\n")             != 0) goto fail;
	}

	if (zenv_atomic_write(path, buf, len, 0600) != 0) goto fail;
	free(buf);
	return 0;

fail:
	free(buf);
	return -1;
}
